// Dear ImGui: standalone example application for SDL3 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important to understand: SDL_Renderer is an _optional_ component of SDL3.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <stdio.h>
#include <SDL3/SDL.h>
#include <thread>
#include <vector>

#include "bh_sim_utils.h"
#include "math.h"

std::thread sim_worker;

void recursive_bh_draw(Node* root, SDL_Renderer* renderer) {
    if (!root) return;

    SDL_FRect rect_;
    rect_.w = ZOOM*(2*root->r);
    rect_.h = ZOOM*(2*root->r);
    rect_.x = WIDTH/2+(root->center.x)*ZOOM-rect_.w/2;
    rect_.y = HEIGTH/2+(root->center.y)*ZOOM-rect_.h/2;
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 127);
    SDL_RenderRect(renderer, &rect_);

    for(int i=0;i<4;i++) {
        if (root->children[i]!=NULL) {
            recursive_bh_draw(root->children[i], renderer);
        }
    }
}

void render_points(SDL_Renderer* renderer, Body** simulation_result, int* N, int* last_ren, int* last_done, Node* root, bool* squares) {

    //printf("%d %d \n", *last_done/60, *last_ren/60);
    if(*last_done > *last_ren) {
        (*last_ren)++;
    }

    if (*squares) {
        recursive_bh_draw(root, renderer);
        // SDL_RenderRects did not improve things
    }

    SDL_FPoint pts[(*N)];
    Body bodies_copy[(*N)];

    memcpy(bodies_copy, simulation_result[*last_ren], sizeof(bodies_copy));

    for(int i=0;i<(*N);i++) {
        pts[i].x = (ZOOM*bodies_copy[i].pos.x)+WIDTH/2;
        pts[i].y = (ZOOM*bodies_copy[i].pos.y)+HEIGTH/2;
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderPoints(renderer, pts, (*N));
}

void init_sim_thread(Body* bodies, Body** simulation_result, int* N, int* last_ren, int* last_done, int* flag, Node* root, double dt, int alg) {

    bodies = (Body*)malloc((*N)*sizeof(Body));

    *last_ren = 0;
    *last_done = 0;
    
    for(int i=0;i<(*N);i++) {
        Body buf;
        buf.pos = (Vec2){0.0,0.0};
        buf.mass = 1.0;
        buf.v = (Vec2){0.0, 0.0};
        bodies[i] = buf;
    }

    simulation_result[0] = bodies;

    *flag = 1;

    sim_worker = std::thread(init_sim, bodies, N, last_done, simulation_result, flag, root, dt, alg);
};

// Main code
int main(int, char**)
{
    // Setup SDL
    // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    // Create window with SDL_Renderer graphics context
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    SDL_Window* window = SDL_CreateWindow("N-Body gravitational simulation", (int)(WIDTH * main_scale), (int)(HEIGTH * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    if (renderer == nullptr)
    {
        SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f); // ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    Uint64 prev = SDL_GetPerformanceCounter(); // used for fps limit

    Uint8 sim = 0; // sim is running actively or not

    // initialization
    Body** simulation_result = (Body**)malloc(10000000 * sizeof(Body*));
    Body* bodies;
    int N;
    double sim_dt;
    int last_ren;
    int last_done;
    int flag = 0;
    Node root;

    bool squares = false;

    double elapsed = 0.0;
    double total_elapsed = 0.0;

    static int slider_n = 5;
    static int dt_gui = 5;
    static int alg_item_selected_idx = 0;

    // Main loop
    bool done = false;
    while (!done)
    {
        // --- FPS LIMIT --- //
        Uint64 now = SDL_GetPerformanceCounter();
        double dt = (now - prev) / (double)SDL_GetPerformanceFrequency();

        elapsed += dt;

        // SDL event queue
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate() function]
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Simulation control panel
        {
            ImGui::Begin("Simulation controls");
            
            if (ImGui::Button("Start") && !flag)
            {
                init_sim_thread(bodies, simulation_result, &N, &last_ren, &last_done, &flag, &root, sim_dt, alg_item_selected_idx);
            }
            ImGui::SameLine();
            if (ImGui::Button("End") && flag)
            {
                flag = 0;
            }
            
            ImGui::SameLine();
            ImGui::Text("Seconds done: %d", last_done/FPS);

            ImGui::SameLine();
            ImGui::Text("Realtime ratio: %f", total_elapsed/MAX(0.00001, last_done/(double)FPS));

            ImGui::SeparatorText("Initial conditions");

            static bool check_lim = true;

            static ImGuiSliderFlags sflags = ImGuiSliderFlags_Logarithmic & ~ImGuiSliderFlags_WrapAround;

            ImGui::SliderInt("Bodies", &slider_n, 0, check_lim?10000:10000000, "%d", sflags);
            ImGui::SameLine();
            ImGui::Checkbox("Lock", &check_lim);
            ImGui::Checkbox("BH viz", &squares);

            static ImGuiSliderFlags zflags = ImGuiSliderFlags_None & ~ImGuiSliderFlags_WrapAround;
            static int slider_z = 250;
            ImGui::SliderInt("Zoom", &slider_z, 0, 500, "%d", zflags);
            ZOOM = slider_z;

            ImGui::SliderInt("dt", &dt_gui, 0, 8, "1e-%d", zflags);

            ImGui::SetNextItemWidth(140);
            const char* alg_items[] = { "naive", "barnes-hut" };


            const char* alg_combo_preview_value = alg_items[alg_item_selected_idx];
            if (ImGui::BeginCombo("Algorithm", alg_combo_preview_value, 0))//flags))
            {
                for (int n = 0; n < IM_ARRAYSIZE(alg_items); n++)
                {
                    const bool is_selected = (alg_item_selected_idx == n);
                    if (ImGui::Selectable(alg_items[n], is_selected))
                        alg_item_selected_idx = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            if (ImGui::Button("Exit") && !flag)
            {
                done = true;
            }
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    
        if(flag) {
            render_points(renderer, simulation_result, &N, &last_ren, &last_done, &root, &squares);
            total_elapsed+=dt;
        } else {
            if(sim_worker.joinable()) {
                sim_worker.join();
            }
            N = slider_n;
            sim_dt = pow(10.0f, -dt_gui);
        }

        SDL_RenderPresent(renderer);

        //-----LIMIT FPS TO 60-----//
        prev = now;
        if (dt < 1.0 / 60.0) {
            SDL_Delay((Uint32)((1.0 / 60.0 - dt) * 1000.0));
        }
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    free(bodies);
    free(simulation_result);
    //free(simulation_result);
    if (sim_worker.joinable()) {
        sim_worker.join();
    }

    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

//   SDL_DestroyTexture(texture); ez még kelleni fog
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
