#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

// C++ includes
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <cfloat>      // for FLT_MIN
#include <algorithm>

// GLib headers – must be outside extern "C"
#include <glib.h>

// Our own C headers – placed inside extern "C" block

#include "../include/core.h"
#include "../include/version.h"
#include "../include/buffer.h"
#include "../include/pipeline.h"
#include "../include/logging.h"
#include "../include/plugin.h"

// ------------------------------------------------------------
// Helper: convert Buffer to std::string
// ------------------------------------------------------------
static std::string buffer_to_string(const Buffer &buf) {
    return std::string((char*)buf.data, buf.len);
}

// ------------------------------------------------------------
// ImGui callback to edit a std::string in place
// ------------------------------------------------------------
static int string_callback(ImGuiInputTextCallbackData* data) {
    std::string* str = (std::string*)data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        // Resize string to fit new size
        str->resize(data->BufTextLen);
        data->Buf = (char*)str->c_str();   // Buf is const char* but we cast
        // In practice, it's safe because ImGui only reads from the buffer,
        // but it may write when the user edits. We'll handle via the edit event.
    } else if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
        // Update string from buffer
        *str = data->Buf;
    }
    return 0;
}

// ------------------------------------------------------------
// Global state for the UI
// ------------------------------------------------------------
static struct {
    // Decode tab
    std::string decode_input;
    std::string decode_output;
    std::string decode_format;
    int decode_format_idx = 0;         // 0 = Auto Detect, 1..n formats
    std::vector<std::string> format_names;   // list of all available decoders

    // Encode tab
    std::string encode_input;
    std::string encode_output;
    int encode_format_idx = 0;         // 0..n-1 for built‑in + plugins with encode
    std::vector<std::string> encode_format_names;

    // Pipeline tab
    std::string pipeline_input;
    std::string pipeline_output;
    std::string pipeline_steps;
    double pipeline_score = 0.0;

    // Settings
    bool dark_theme = true;
    bool auto_decode = false;           // auto‑decode when input changes
    bool show_log = false;

    // Cached plugin lists (performance)
    std::vector<Plugin*> cached_plugins;
    std::vector<Plugin*> cached_encode_plugins;
} g_ui;

// ------------------------------------------------------------
// ImGui getter for combo boxes (returns const char*)
// ------------------------------------------------------------
static const char* vector_getter(void* data, int idx) {
    auto* vec = static_cast<std::vector<std::string>*>(data);
    if (idx < 0 || idx >= (int)vec->size()) return nullptr;
    return (*vec)[idx].c_str();
}

// ------------------------------------------------------------
// Refresh format lists from plugin manager (and cache plugins)
// ------------------------------------------------------------
static void refresh_format_lists() {
    g_ui.format_names.clear();
    g_ui.format_names.push_back("Auto Detect");
    g_ui.format_names.push_back("Base64");
    g_ui.format_names.push_back("Hex");
    g_ui.format_names.push_back("Binary");
    g_ui.format_names.push_back("Morse");

    g_ui.encode_format_names.clear();
    g_ui.encode_format_names.push_back("Base64");
    g_ui.encode_format_names.push_back("Hex");
    g_ui.encode_format_names.push_back("Binary");
    g_ui.encode_format_names.push_back("Morse");

    g_ui.cached_plugins.clear();
    g_ui.cached_encode_plugins.clear();

    PluginManager *pm = core_get_plugin_manager();
    if (pm) {
        GList *plugins = plugin_manager_list(pm);
        for (GList *iter = plugins; iter; iter = iter->next) {
            Plugin *p = (Plugin*)iter->data;
            g_ui.cached_plugins.push_back(p);
            g_ui.format_names.push_back(p->name);
            if (p->encode) {
                g_ui.cached_encode_plugins.push_back(p);
                g_ui.encode_format_names.push_back(p->name);
            }
        }
        g_list_free(plugins);
    }
}

// ------------------------------------------------------------
// Decode the current input
// ------------------------------------------------------------
static void do_decode() {
    if (g_ui.decode_input.empty()) {
        g_ui.decode_output.clear();
        g_ui.decode_format.clear();
        return;
    }

    int idx = g_ui.decode_format_idx;
    if (idx < 5) {
        CoreMode mode;
        switch (idx) {
            case 0: mode = CORE_MODE_AUTO; break;
            case 1: mode = CORE_MODE_BASE64; break;
            case 2: mode = CORE_MODE_HEX; break;
            case 3: mode = CORE_MODE_BINARY; break;
            case 4: mode = CORE_MODE_MORSE; break;
            default: mode = CORE_MODE_AUTO;
        }
        DecodeResult *res = core_decode_with_mode(g_ui.decode_input.c_str(), mode);
        if (res && res->error_code == ERROR_OK) {
            g_ui.decode_output = res->output;
            g_ui.decode_format = res->format;
        } else {
            g_ui.decode_output = "Decode failed";
            g_ui.decode_format = "Error";
        }
        decode_result_free(res);
    } else {
        int plugin_idx = idx - 5;
        if (plugin_idx >= 0 && plugin_idx < (int)g_ui.cached_plugins.size()) {
            Plugin *p = g_ui.cached_plugins[plugin_idx];
            if (p && p->decode_single) {
                Buffer in = buffer_new((const unsigned char*)g_ui.decode_input.c_str(),
                                        g_ui.decode_input.size());
                Buffer out = p->decode_single(in);
                buffer_free(&in);
                if (out.data) {
                    g_ui.decode_output = buffer_to_string(out);
                    g_ui.decode_format = p->name;
                    buffer_free(&out);
                } else {
                    g_ui.decode_output = "Plugin decode failed";
                    g_ui.decode_format = "Error";
                }
            } else {
                g_ui.decode_output = "Plugin decode not available";
                g_ui.decode_format = "Error";
            }
        } else {
            g_ui.decode_output = "Plugin index out of range";
            g_ui.decode_format = "Error";
        }
    }
}

// ------------------------------------------------------------
// Encode the current input
// ------------------------------------------------------------
static void do_encode() {
    if (g_ui.encode_input.empty()) {
        g_ui.encode_output.clear();
        return;
    }

    int idx = g_ui.encode_format_idx;
    if (idx < 4) {
        CoreMode mode;
        switch (idx) {
            case 0: mode = CORE_MODE_BASE64; break;
            case 1: mode = CORE_MODE_HEX; break;
            case 2: mode = CORE_MODE_BINARY; break;
            case 3: mode = CORE_MODE_MORSE; break;
            default: mode = CORE_MODE_BASE64;
        }
        EncodeResult *res = core_encode(g_ui.encode_input.c_str(), mode);
        if (res && res->error_code == ERROR_OK) {
            g_ui.encode_output = res->output;
        } else {
            g_ui.encode_output = "Encode failed";
        }
        encode_result_free(res);
    } else {
        int plugin_idx = idx - 4;
        if (plugin_idx >= 0 && plugin_idx < (int)g_ui.cached_encode_plugins.size()) {
            Plugin *p = g_ui.cached_encode_plugins[plugin_idx];
            if (p && p->encode) {
                Buffer in = buffer_new((const unsigned char*)g_ui.encode_input.c_str(),
                                        g_ui.encode_input.size());
                Buffer out = p->encode(in);
                buffer_free(&in);
                if (out.data) {
                    g_ui.encode_output = buffer_to_string(out);
                    buffer_free(&out);
                } else {
                    g_ui.encode_output = "Plugin encode failed";
                }
            } else {
                g_ui.encode_output = "Plugin encode not available";
            }
        } else {
            g_ui.encode_output = "Plugin index out of range";
        }
    }
}

// ------------------------------------------------------------
// Run the pipeline on given input
// ------------------------------------------------------------
static void do_pipeline() {
    if (g_ui.pipeline_input.empty()) {
        g_ui.pipeline_output.clear();
        g_ui.pipeline_steps.clear();
        g_ui.pipeline_score = 0.0;
        return;
    }

    Buffer in = buffer_new((const unsigned char*)g_ui.pipeline_input.c_str(),
                            g_ui.pipeline_input.size());
    GList *candidates = pipeline_beam_search(in, 3, 5);
    buffer_free(&in);

    if (candidates) {
        Candidate *best = (Candidate*)candidates->data;
        g_ui.pipeline_output = buffer_to_string(best->buf);
        g_ui.pipeline_score = best->score;

        std::string steps_str;
        for (GList *s = best->steps; s; s = s->next) {
            if (!steps_str.empty()) steps_str += " → ";
            steps_str += (char*)s->data;
        }
        g_ui.pipeline_steps = steps_str;
    } else {
        g_ui.pipeline_output = "Pipeline found nothing";
        g_ui.pipeline_steps.clear();
        g_ui.pipeline_score = 0.0;
    }
    candidate_list_free(candidates);
}

// ------------------------------------------------------------
// Main UI rendering
// ------------------------------------------------------------
static void render_ui() {
    // Theme handling – only change when needed
    static bool last_theme = true;
    if (last_theme != g_ui.dark_theme) {
        last_theme = g_ui.dark_theme;
        if (g_ui.dark_theme)
            ImGui::StyleColorsDark();
        else
            ImGui::StyleColorsLight();
    }

    // Main window with a simple tab bar
    if (ImGui::Begin("Auto Decoder Pro")) {
        if (ImGui::BeginTabBar("MainTabBar")) {
            // Decode tab
            if (ImGui::BeginTabItem("Decode")) {
                // Input text area
                if (ImGui::InputTextMultiline("Input", (char*)g_ui.decode_input.c_str(),
                                              g_ui.decode_input.capacity(),
                                              ImVec2(-FLT_MIN, 100),
                                              ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackEdit,
                                              string_callback, &g_ui.decode_input)) {
                    if (g_ui.auto_decode) do_decode();
                }

                // Combo box
                if (ImGui::Combo("Format", &g_ui.decode_format_idx,
                                 vector_getter, (void*)&g_ui.format_names,
                                 g_ui.format_names.size())) {
                    if (g_ui.auto_decode) do_decode();
                }

                // Buttons
                if (ImGui::Button("Decode")) do_decode();
                ImGui::SameLine();
                if (ImGui::Button("Copy Output")) {
                    ImGui::SetClipboardText(g_ui.decode_output.c_str());
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear")) {
                    g_ui.decode_input.clear();
                    g_ui.decode_output.clear();
                    g_ui.decode_format.clear();
                }

                ImGui::Text("Detected format: %s", g_ui.decode_format.c_str());

                // Output area – read‑only
                ImGui::InputTextMultiline("Output", (char*)g_ui.decode_output.c_str(),
                                          g_ui.decode_output.size(),
                                          ImVec2(-FLT_MIN, 150),
                                          ImGuiInputTextFlags_ReadOnly);

                // Drag & drop support (text)
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXT")) {
                        g_ui.decode_input = (const char*)payload->Data;
                        do_decode();
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::EndTabItem();
            }

            // Encode tab
            if (ImGui::BeginTabItem("Encode")) {
                if (ImGui::InputTextMultiline("Input", (char*)g_ui.encode_input.c_str(),
                                              g_ui.encode_input.capacity(),
                                              ImVec2(-FLT_MIN, 100),
                                              ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackEdit,
                                              string_callback, &g_ui.encode_input)) {
                    if (g_ui.auto_decode) do_encode();
                }

                if (ImGui::Combo("Format", &g_ui.encode_format_idx,
                                 vector_getter, (void*)&g_ui.encode_format_names,
                                 g_ui.encode_format_names.size())) {
                    if (g_ui.auto_decode) do_encode();
                }

                if (ImGui::Button("Encode")) do_encode();
                ImGui::SameLine();
                if (ImGui::Button("Copy Output")) {
                    ImGui::SetClipboardText(g_ui.encode_output.c_str());
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear")) {
                    g_ui.encode_input.clear();
                    g_ui.encode_output.clear();
                }

                ImGui::InputTextMultiline("Output", (char*)g_ui.encode_output.c_str(),
                                          g_ui.encode_output.size(),
                                          ImVec2(-FLT_MIN, 150),
                                          ImGuiInputTextFlags_ReadOnly);

                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXT")) {
                        g_ui.encode_input = (const char*)payload->Data;
                        do_encode();
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::EndTabItem();
            }

            // Pipeline tab
            if (ImGui::BeginTabItem("Pipeline")) {
                if (ImGui::InputTextMultiline("Input", (char*)g_ui.pipeline_input.c_str(),
                                              g_ui.pipeline_input.capacity(),
                                              ImVec2(-FLT_MIN, 100),
                                              ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackEdit,
                                              string_callback, &g_ui.pipeline_input)) {
                    // optional: auto-run pipeline on edit
                }

                if (ImGui::Button("Run Pipeline")) do_pipeline();
                ImGui::SameLine();
                if (ImGui::Button("Copy Result")) {
                    ImGui::SetClipboardText(g_ui.pipeline_output.c_str());
                }

                ImGui::Separator();
                ImGui::Text("Score: %.3f", g_ui.pipeline_score);
                ImGui::Text("Steps: %s", g_ui.pipeline_steps.c_str());
                ImGui::InputTextMultiline("Result", (char*)g_ui.pipeline_output.c_str(),
                                          g_ui.pipeline_output.size(),
                                          ImVec2(-FLT_MIN, 200),
                                          ImGuiInputTextFlags_ReadOnly);

                ImGui::EndTabItem();
            }

            // Settings tab
            if (ImGui::BeginTabItem("Settings")) {
                ImGui::Checkbox("Dark Theme", &g_ui.dark_theme);
                ImGui::Checkbox("Auto-decode on edit", &g_ui.auto_decode);
                ImGui::Checkbox("Show Log Window", &g_ui.show_log);
                if (ImGui::Button("Refresh Formats")) {
                    refresh_format_lists();
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear Cache")) {
                    core_cache_clear();
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    // Log window (if enabled)
    if (g_ui.show_log) {
        ImGui::Begin("Log", &g_ui.show_log);
        ImGui::Text("(Logging not yet implemented in this demo)");
        ImGui::End();
    }
}

// ------------------------------------------------------------
// Main entry point
// ------------------------------------------------------------
int main(int argc, char** argv) {
    (void)argc; (void)argv;

    core_init();
    refresh_format_lists();

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* window = SDL_CreateWindow(
        "Auto Decoder Pro",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // vsync

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io; // silence unused variable warning

    ImGui::StyleColorsDark();

    // Setup backend
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 150");

    // Main loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                running = false;
        }

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(); // note: no window argument (matches our ImGui version)
        ImGui::NewFrame();

        render_ui();

        // Render
        ImGui::Render();
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    core_cleanup();

    return 0;
}