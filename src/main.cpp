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

#include "bh_sim_utils.h"
#include "math.h"

#define PI 3.14159265358979323846

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

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
    constexpr int WIDTH = 1280;
    constexpr int HEIGTH = 720;
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

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
//    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    
    Body bodies[5];
    
    for(int i=0;i<5;i++){
        float test[2];
        test[0] = 0.0+100.0*cos(i*2*PI/5);
        test[1] = 0.0+100.0*sin(i*2*PI/5);
        bodies[i].pos = (Vec2){test[0], test[1]};
        bodies[i].v = (Vec2){0.0, 0.0};
        bodies[i].mass = 1000.0;
    }



    Uint64 prev = SDL_GetPerformanceCounter();

    Uint8 sim = 0;

    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        Uint64 now = SDL_GetPerformanceCounter();
        double dt = (now - prev) / (double)SDL_GetPerformanceFrequency();

        // printf("%f\n",dt);
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        // [If using SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your SDL_AppEvent() function]
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

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Simulation controls");
            
            static int start_clicked = 0;
            if (ImGui::Button("Start"))
                start_clicked++;
            if (start_clicked & 1)
            {
                sim = 1;
                ImGui::SameLine();
                ImGui::Text("Simulation started!");
            }
            ImGui::SameLine();
            static int end_clicked = 0;
            if (ImGui::Button("End"))
                end_clicked++;
            if (end_clicked & 1)
            {
                ImGui::SameLine();
                ImGui::Text("Simulation ended!");
            }
            
            ImGui::SameLine();
            ImGui::Text("FPS: 0");

            ImGui::SameLine();
            ImGui::Text("Avg. Frame time: 0ns");

            ImGui::SeparatorText("Initial conditions");

//            ImGui::Text("Bodies: ");
            //ImGui::SameLine();
            ImGui::SetNextItemWidth(140);
            static float f1 = 1.e1f;
            ImGui::InputFloat("Bodies", &f1, 0.0f, 0.0f, "%e");
//            ImGui::SameLine(); HelpMarker(
//                "You can input value using the scientific notation,\n"
//                "  e.g. \"1e+8\" becomes \"100000000\".");


//            ImGui::Text("Spawn: ");
//            ImGui::SameLine();
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


//            ImGui::Text("Algo: ");
//            ImGui::SameLine();
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

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        
        
        constexpr float G = 10000.0;

        if (sim)
        {
            for(int i=0;i<5;i++) {
                // F = ma
                // a = m/F
                // F = G m1 m2 / r^2
                
                Vec2 acc = {0.0,0.0};

                for(int j=0;j<5;j++) {
                    if (i!=j){
                        double dx,dy,dist;
                        dx = bodies[j].pos.x-bodies[i].pos.x;
                        dy = bodies[j].pos.y-bodies[i].pos.y;
                        dist = sqrt(dx*dx+dy*dy);
                        if (dist > 1e-9) {
                            double invDist3 = 1.0 / (dist * dist * dist);
                            // Acceleration contribution: G * m_j * (r_ij) / |r_ij|^3
                            acc.x += G * bodies[j].mass * dx * invDist3;
                            acc.y += G * bodies[j].mass * dy * invDist3;
                            //printf("%f %f\n", acc.x, acc.y);
                        }
                    }
                }

                Vec2 v1 = {bodies[i].v.x+acc.x*dt,bodies[i].v.y+acc.y*dt};
                bodies[i].pos = {bodies[i].pos.x+v1.x*dt,bodies[i].pos.y+v1.y*dt};
            }
        }
        
        SDL_FPoint pts[5];
        // if atomic flag, fetch Body bodies[n] from memory, n.pos -> SDL_Point pts[n] .x/.y, then SDL_SetRenderDrawColor to white, and SDL_RenderDrawPoints(renderer, pts, n);
        for(int i=0;i<5;i++){
            pts[i].x = bodies[i].pos.x+WIDTH/2;
            pts[i].y = bodies[i].pos.y+HEIGTH/2;
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); /* white points */
        SDL_RenderPoints(renderer, pts, 5);
        
        SDL_RenderPresent(renderer);

        prev = now;

        if (dt < 1.0 / 60.0) {
            SDL_Delay((Uint32)((1.0 / 60.0 - dt) * 1000.0));
        }
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
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
