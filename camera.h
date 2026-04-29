#ifndef CAMERA_H
#define CAMERA_H

#include "display.h"
#include "hittable.h"
#include "material.h"
#include <vector>

class camera {
    public:
    double aspect_ratio = 16.0 / 9.0;
    int image_width = 400;
    int samples_per_pixel = 10;
    int max_depth = 10;

    double vfov = 90;

    point3 lookfrom = point3(0,0,0);
    point3 lookat = point3(0,0,-1);
    vec3 vup = vec3(0,1,0);

    double defocus_angle = 0;
    double focus_dist = 10;

    // Computed by init() — exposed so the main loop can size its buffers
    // and create the Display before calling render_pass().
    int image_height = 0;

    // Recomputes all derived camera state from the public fields above.
    // Must be called once after setting parameters and again after any change.
    void init() {
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;
        pixel_samples_scale = 1.0 / samples_per_pixel;

        centre = lookfrom;

        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta/2);
        auto viewport_height = 2 * h * focus_dist;
        auto viewport_width = viewport_height * (double(image_width)/image_height);

        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup,w));
        v = cross(w,u);

        auto viewport_u = viewport_width * u;
        auto viewport_v = viewport_height * -v;

        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        auto viewport_upper_left = centre
                                 - (focus_dist * w)
                                 - viewport_u/2 - viewport_v/2;

        pixe100_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        auto defocus_radius = focus_dist *
                              std::tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    // Adds one sample per pixel into accum[]. Call init() before the first pass.
    void render_pass(std::vector<colour>& accum, const hittable& world) {
        #pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < image_height; j++)
            for (int i = 0; i < image_width; i++)
                accum[j * image_width + i] += ray_colour(get_ray(i, j), max_depth, world);
    }

    // Converts the accumulated sum (after `pass` samples) into a linear-float
    // staging buffer ready for glTexSubImage2D. Gamma correction stays on the GPU.
    void accumulate_to_staging(const std::vector<colour>& accum,
                               std::vector<float>& staging, int pass) {
        const float scale = 1.0f / static_cast<float>(pass);
        const int   n     = image_height * image_width;
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++) {
            staging[i*3+0] = static_cast<float>(accum[i].x()) * scale;
            staging[i*3+1] = static_cast<float>(accum[i].y()) * scale;
            staging[i*3+2] = static_cast<float>(accum[i].z()) * scale;
        }
    }

    void render(const hittable& world){
        init();

        std::vector<colour> framebuffer(image_height * image_width);

        std::clog << "Rendering " << image_width << "x" << image_height
                  << " @ " << samples_per_pixel << " spp...\n";

        #pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < image_height; j++) {
            for (int i = 0; i < image_width; i++) {
                colour pixel_colour(0, 0, 0);
                for (int sample = 0; sample < samples_per_pixel; sample++)
                    pixel_colour += ray_colour(get_ray(i, j), max_depth, world);
                framebuffer[j * image_width + i] = pixel_samples_scale * pixel_colour;
            }
        }

        std::cout << "P3\n" << image_width << " " << image_height << "\n255\n";
        for (const auto& px : framebuffer)
            write_colour(std::cout, px);

        std::clog << "Done!\n";
    }

    // Progressive window render — opens an OpenGL window and refines the image
    // one sample per pass, updating the display after each pass so you see the
    // image converge in real time. Press Q or ESC to close when done.
    void render_in_window(const hittable& world) {
        init();

        Display display(image_width, image_height, "Raytracer");

        const int n_pixels = image_height * image_width;
        std::vector<colour> accum(n_pixels, colour(0, 0, 0));
        std::vector<float>  staging(n_pixels * 3, 0.0f);

        display.present(staging.data());  // show black window immediately on launch

        for (int pass = 1; pass <= samples_per_pixel && !display.should_close(); pass++) {

            // One sample added to every pixel across all threads
            #pragma omp parallel for schedule(dynamic)
            for (int j = 0; j < image_height; j++) {
                for (int i = 0; i < image_width; i++) {
                    accum[j * image_width + i] += ray_colour(get_ray(i, j), max_depth, world);
                }
            }

            // Convert accumulated linear-double values to linear-float for GPU upload.
            // Gamma correction is deferred to the fragment shader.
            const float scale = 1.0f / static_cast<float>(pass);
            #pragma omp parallel for schedule(static)
            for (int i = 0; i < n_pixels; i++) {
                staging[i*3+0] = static_cast<float>(accum[i].x()) * scale;
                staging[i*3+1] = static_cast<float>(accum[i].y()) * scale;
                staging[i*3+2] = static_cast<float>(accum[i].z()) * scale;
            }

            display.present(staging.data());
            std::clog << "\rPass " << pass << " / " << samples_per_pixel << std::flush;
        }

        std::clog << "\nDone — press Q or ESC to close.\n";

        while (!display.should_close())
            display.present(staging.data());
    }

    private:
        double pixel_samples_scale = 0;
        point3 centre;
        point3 pixe100_loc;
        vec3 pixel_delta_u;
        vec3 pixel_delta_v;
        vec3 u, v, w;
        vec3 defocus_disk_u;
        vec3 defocus_disk_v;

    ray get_ray(int i,int j) const{
        auto offset =  sample_square();
        auto pixel_sample = pixe100_loc
                        + ((i + offset.x()) * pixel_delta_u)
                        + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = (defocus_angle <= 0) ? centre : defocus_disk_sample();
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin,ray_direction);
    }

    vec3 sample_square() const{
        return vec3(random_double() - 0.5,random_double() - 0.5,0);
    }

    point3 defocus_disk_sample() const{
        auto p = random_in_unit_disk();
        return centre + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }

    colour ray_colour(const ray& r_in, int depth, const hittable& world) const {
        colour accumulated(1, 1, 1);
        ray current = r_in;

        for (int d = 0; d < depth; d++) {
            hit_record rec;
            if (!world.hit(current, interval(0.001, infinity), rec)) {
                vec3 unit_dir = unit_vector(current.direction());
                auto a = 0.5 * (unit_dir.y() + 1.0);
                colour sky = (1.0 - a) * colour(1.0, 1.0, 1.0) + a * colour(0.5, 0.7, 1.0);
                return accumulated * sky;
            }

            ray scattered;
            colour attenuation;
            if (!rec.mat->scatter(current, rec, attenuation, scattered))
                return colour(0, 0, 0);

            accumulated = accumulated * attenuation;
            current = scattered;
        }

        return colour(0, 0, 0);
    }
};


#endif