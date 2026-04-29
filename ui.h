#ifndef UI_H
#define UI_H

#include <epoxy/gl.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "camera_state.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <string>

// ---------------------------------------------------------------------------
// RenderState — tracks which phase the renderer is in and owns debounce timing.
//
// Debouncing strategy: when the user drags a slider, mark_dirty() is called on
// every value change. Each call resets a 500 ms countdown. Only after 500 ms of
// silence (no new changes) does the render restart. This prevents re-launching a
// full multi-second render on every intermediate slider position, while still
// feeling snappy once the user releases the control.
// ---------------------------------------------------------------------------
struct RenderState {
    enum class Phase { Idle, Debouncing, Rendering, Done };

    Phase phase        = Phase::Idle;
    int   current_pass = 0;
    int   total_passes = 0;

    void mark_dirty() {
        dirty_time   = std::chrono::steady_clock::now();
        phase        = Phase::Debouncing;
        current_pass = 0;
    }

    bool debounce_elapsed() const {
        return std::chrono::steady_clock::now() - dirty_time >= DEBOUNCE;
    }

    int ms_remaining() const {
        auto rem = DEBOUNCE - (std::chrono::steady_clock::now() - dirty_time);
        return std::max(0,
            static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(rem).count()));
    }

    float progress() const {
        return total_passes > 0
            ? static_cast<float>(current_pass) / static_cast<float>(total_passes)
            : 0.0f;
    }

private:
    std::chrono::steady_clock::time_point dirty_time;
    static constexpr std::chrono::milliseconds DEBOUNCE{500};
};

// ---------------------------------------------------------------------------
// ImGui lifecycle helpers
// ---------------------------------------------------------------------------
inline void imgui_init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

inline void imgui_shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// ---------------------------------------------------------------------------
// draw_ui — renders the camera control panel on top of the raytraced image.
// Returns true if any parameter changed this frame (triggers debounce timer).
// ---------------------------------------------------------------------------
inline bool draw_ui(CameraState& state, RenderState& rs) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    bool changed = false;

    ImGui::SetNextWindowPos({10.0f, 10.0f}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({300.0f, 0.0f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.88f);

    ImGui::Begin("Camera Controls", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // ---- Position --------------------------------------------------------
    ImGui::SeparatorText("Position");
    changed |= ImGui::SliderFloat("X##pos", &state.pos_x,   -20.0f, 20.0f, "%.1f");
    changed |= ImGui::SliderFloat("Y##pos", &state.pos_y,    -5.0f, 20.0f, "%.1f");
    changed |= ImGui::SliderFloat("Z##pos", &state.pos_z,   -20.0f, 20.0f, "%.1f");

    // ---- Look Target -----------------------------------------------------
    ImGui::SeparatorText("Look Target");
    changed |= ImGui::SliderFloat("X##tgt", &state.target_x, -5.0f,  5.0f, "%.1f");
    changed |= ImGui::SliderFloat("Y##tgt", &state.target_y, -2.0f,  5.0f, "%.1f");
    changed |= ImGui::SliderFloat("Z##tgt", &state.target_z, -5.0f,  5.0f, "%.1f");

    // ---- Lens ------------------------------------------------------------
    ImGui::SeparatorText("Lens");
    changed |= ImGui::SliderFloat("FOV",     &state.vfov,          10.0f, 120.0f, "%.0f deg");
    changed |= ImGui::SliderFloat("Focus",   &state.focus_dist,     0.1f,  30.0f, "%.1f");
    changed |= ImGui::SliderFloat("Defocus", &state.defocus_angle,  0.0f,   5.0f, "%.2f deg");

    // ---- Quality ---------------------------------------------------------
    ImGui::SeparatorText("Quality");
    changed |= ImGui::SliderInt("Samples",   &state.samples_per_pixel,  1, 200);
    changed |= ImGui::SliderInt("Max Depth", &state.max_depth,          1,  50);

    // ---- Presets ---------------------------------------------------------
    ImGui::SeparatorText("Presets");
    if (ImGui::Button("Default"))  { state = presets::default_view(); changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Wide"))     { state = presets::wide_angle();   changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Close-up")) { state = presets::close_up();     changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Top"))      { state = presets::overhead();     changed = true; }

    // ---- Render Status ---------------------------------------------------
    ImGui::SeparatorText("Status");
    switch (rs.phase) {
        case RenderState::Phase::Idle:
            ImGui::TextDisabled("Idle");
            break;
        case RenderState::Phase::Debouncing:
            ImGui::TextColored({1.0f, 0.8f, 0.0f, 1.0f},
                "Waiting %d ms...", rs.ms_remaining());
            break;
        case RenderState::Phase::Rendering: {
            std::string lbl = std::to_string(rs.current_pass) + " / " +
                              std::to_string(rs.total_passes) + " spp";
            ImGui::ProgressBar(rs.progress(), {-1.0f, 0.0f}, lbl.c_str());
            break;
        }
        case RenderState::Phase::Done:
            ImGui::TextColored({0.4f, 1.0f, 0.4f, 1.0f},
                "Done  (%d spp)", rs.total_passes);
            break;
    }

    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return changed;
}

#endif
