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

void render_points(SDL_Renderer* renderer, Body* simulation_result[10000], int* last_ren, int* last_done) {

    printf("%d %d \n", *last_done/60, *last_ren/60);
    if(*last_done > *last_ren) {
        (*last_ren)++;
    }

    SDL_FPoint pts[N];
    Body bodies_copy[N];

    memcpy(bodies_copy, simulation_result[*last_ren], sizeof(bodies_copy));

    for(int i=0;i<N;i++) {
        pts[i].x = bodies_copy[i].pos.x+WIDTH/2;
        pts[i].y = bodies_copy[i].pos.y+HEIGTH/2;
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); /* white points */
    SDL_RenderPoints(renderer, pts, N);
}

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

    int initialized = 0;

    int ts_done = 0;
    Body* simulation_result[100000];
    int last_ren = 0;
    int last_done = 0;

    Body* bodies = (Body*)malloc(N*sizeof(Body));
    
    for(int i=0;i<N;i++) {
        Body buf;
        buf.pos = (Vec2){0.0,0.0};
        buf.mass = 1.0;
        buf.v = (Vec2){0.0, 0.0};
        bodies[i] = buf;
    }

    simulation_result[0] = bodies;

    int flag = 1;

//    Uint64 elapsed = SDL_GetPerformanceCounter();
    double elapsed = 0.0;

    // Main loop
    bool done = false;
    while (!done)
    {
        // --- FPS LIMIT --- //
        Uint64 now = SDL_GetPerformanceCounter();
        double dt = (now - prev) / (double)SDL_GetPerformanceFrequency();

        elapsed += dt;
//        printf("%f\n", elapsed);

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
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Simulation controls");
            
            static int start_clicked = 0;
            if (ImGui::Button("Start"))
                start_clicked++;
            if (start_clicked & 1 && !sim)
            {
                sim = 1;
                sim_worker = std::thread(init_sim, bodies, &last_done, simulation_result, &flag);

//                ImGui::SameLine();
//                ImGui::Text("Simulation started!");
            }
            ImGui::SameLine();
            static int end_clicked = 0;
            if (ImGui::Button("End"))
                end_clicked++;
            if (end_clicked & 1)
            {
                flag = 0;
                sim = -1;
//                ImGui::SameLine();
//                ImGui::Text("Simulation ended!");
            }
            
            ImGui::SameLine();
            ImGui::Text("%d/1000000", last_done);

            ImGui::SameLine();
            ImGui::Text("Avg. Frame time: 0ns");

            ImGui::SeparatorText("Initial conditions");

            ImGui::SetNextItemWidth(140);
            static float f1 = 1.e1f;
            ImGui::InputFloat("Bodies", &f1, 0.0f, 0.0f, "%e");

            ImGui::SetNextItemWidth(140);
            static ImGuiComboFlags flags = 0;

            const char* spawn_items[] = { "ellipse", "circle" };
            static int spawn_item_selected_idx = 0;

            const char* spawn_combo_preview_value = spawn_items[spawn_item_selected_idx];
            if (ImGui::BeginCombo("Spawn function", spawn_combo_preview_value, flags))
            {
                for (int n = 0; n < IM_ARRAYSIZE(spawn_items); n++)
                {
                    const bool is_selected = (spawn_item_selected_idx == n);
                    if (ImGui::Selectable(spawn_items[n], is_selected))
                        spawn_item_selected_idx = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::SetNextItemWidth(140);
            const char* alg_items[] = { "naive", "naive parallel", "barnes-hut", "barnes-hut parallel"};
            static int alg_item_selected_idx = 0;

            const char* alg_combo_preview_value = alg_items[alg_item_selected_idx];
            if (ImGui::BeginCombo("Algorithm", alg_combo_preview_value, flags))
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

            ImGui::SeparatorText("Hardware");

            ImGui::SetNextItemWidth(140);
            const char* hw_items[] = { "local", "remote" };
            static int hw_item_selected_idx = 0;

            const char* hw_combo_preview_value = hw_items[hw_item_selected_idx];
            if (ImGui::BeginCombo("Simulation hardware", hw_combo_preview_value, flags))
            {
                for (int n = 0; n < IM_ARRAYSIZE(hw_items); n++)
                {
                    const bool is_selected = (hw_item_selected_idx == n);
                    if (ImGui::Selectable(hw_items[n], is_selected))
                        hw_item_selected_idx = n;

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }


            static int save_init_clicked = 0;
            if (ImGui::Button("Save initial"))
                save_init_clicked++;
            if (save_init_clicked & 1)
            {
                ImGui::SameLine();
                ImGui::Text("Ph");
            }
            
            ImGui::SameLine();

            static int open_res_clicked = 0;
            if (ImGui::Button("Open result"))
                open_res_clicked++;
            if (open_res_clicked & 1)
            {
                ImGui::SameLine();
                ImGui::Text("Ph");
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    
        if(sim)
            render_points(renderer, simulation_result, &last_ren, &last_done);

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
