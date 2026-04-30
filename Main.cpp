#include "raytrace.h"

#include <ctime>
#include <fstream>

#include "bvh.h"
#include "camera.h"
#include "camera_state.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "ui.h"

static void export_ppm(const std::vector<float>& staging, int width, int height) {
    std::time_t t = std::time(nullptr);
    char filename[32];
    std::strftime(filename, sizeof(filename), "render_%Y%m%d_%H%M%S.ppm", std::localtime(&t));

    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "P3\n" << width << ' ' << height << "\n255\n";
    for (int i = 0; i < width * height; i++) {
        // staging holds linear-light values; apply sqrt gamma-2.0 correction
        float r = staging[i*3+0];
        float g = staging[i*3+1];
        float b = staging[i*3+2];
        r = (r > 0.0f) ? std::sqrt(r) : 0.0f;
        g = (g > 0.0f) ? std::sqrt(g) : 0.0f;
        b = (b > 0.0f) ? std::sqrt(b) : 0.0f;
        auto to_byte = [](float v) {
            return static_cast<int>(256.0f * std::min(std::max(v, 0.0f), 0.999f));
        };
        file << to_byte(r) << ' ' << to_byte(g) << ' ' << to_byte(b) << '\n';
    }
}

int main() {
    // ---- Scene setup (unchanged) ----------------------------------------
    hittable_list world;

    auto ground_material = make_shared<lambertian>(colour(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    auto albedo = colour::random() * colour::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    auto albedo = colour::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(colour(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(colour(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    bvh_node bvh(world);

    // ---- Camera + initial state -----------------------------------------
    CameraState state;   // defaults match the original scene

    camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width  = 1200;
    cam.vup          = vec3(0, 1, 0);

    // Applies CameraState fields onto the camera object and calls init().
    auto apply_and_init = [&]() {
        cam.vfov              = state.vfov;
        cam.lookfrom          = point3(state.pos_x,    state.pos_y,    state.pos_z);
        cam.lookat            = point3(state.target_x, state.target_y, state.target_z);
        cam.defocus_angle     = state.defocus_angle;
        cam.focus_dist        = state.focus_dist;
        cam.samples_per_pixel = state.samples_per_pixel;
        cam.max_depth         = state.max_depth;
        cam.init();
    };

    apply_and_init();

    // ---- Window + ImGui -------------------------------------------------
    Display display(cam.image_width, cam.image_height, "Raytracer");
    imgui_init(display.window);

    // ---- Render buffers -------------------------------------------------
    const int n = cam.image_height * cam.image_width;
    std::vector<colour> accum(n, colour(0, 0, 0));
    std::vector<float>  staging(n * 3, 0.0f);

    // ---- Render state ---------------------------------------------------
    RenderState rs;
    rs.total_passes = state.samples_per_pixel;
    rs.phase        = RenderState::Phase::Rendering;   // start rendering immediately

    // Resets accumulation buffers and restarts from pass 0 with current state.
    auto restart_render = [&]() {
        apply_and_init();
        std::fill(accum.begin(),   accum.end(),   colour(0, 0, 0));
        std::fill(staging.begin(), staging.end(), 0.0f);
        rs.current_pass = 0;
        rs.total_passes = state.samples_per_pixel;
        rs.phase        = RenderState::Phase::Rendering;
    };

    // ---- Main loop ------------------------------------------------------
    while (!display.should_close()) {

        // 1. Debounce: if the user stopped moving sliders, restart the render
        if (rs.phase == RenderState::Phase::Debouncing && rs.debounce_elapsed())
            restart_render();

        // 2. Render one sample-per-pixel pass across all OMP threads
        if (rs.phase == RenderState::Phase::Rendering) {
            rs.current_pass++;
            cam.render_pass(accum, bvh);
            cam.accumulate_to_staging(accum, staging, rs.current_pass);
            display.upload(staging.data());
            if (rs.current_pass >= rs.total_passes)
                rs.phase = RenderState::Phase::Done;
        }

        // 3. Draw frame: raytracer texture first, ImGui panel on top
        glClear(GL_COLOR_BUFFER_BIT);
        display.draw_quad();

        if (draw_ui(state, rs))
            rs.mark_dirty();   // any slider/button change → debounce timer

        if (rs.export_requested) {
            rs.export_requested = false;
            export_ppm(staging, cam.image_width, cam.image_height);
        }

        display.swap_and_poll();
    }

    imgui_shutdown();
}
