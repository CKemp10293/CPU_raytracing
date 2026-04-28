#ifndef CAMERA_H
#define CAMERA_H

#include "hittable.h"
class camera {
    public:
    double aspect_ratio = 16.0 / 9.0;
    int image_width = 400;
    int samples_per_pixel = 10;
    int max_depth = 10;

    void render(const hittable& world){
        init();

        std::cout << "P3\n" << image_width << " " << image_height << "\n255\n";

        for (int j = 0; j < image_height; j++)
        {
        std::clog << "\rScanlines remaining: " << (image_height - j) << " " << std::flush; 
            for (int i = 0; i < image_width; i++)
            {
                colour pixel_colour(0,0,0);
                for (int sample = 0; sample < samples_per_pixel; sample++)
                {
                    ray r =  get_ray(i,j);
                    pixel_colour += ray_colour(r,max_depth,world);
                }
                write_colour(std::cout,pixel_samples_scale * pixel_colour);
            }
        }
        std::clog << "\rDone!                \n";
    }

    private:
        int image_height;
        double pixel_samples_scale;
        point3 centre;
        point3 pixe100_loc;
        vec3 pixel_delta_u;
        vec3 pixel_delta_v;


    void init(){
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;
        pixel_samples_scale = 1.0 / samples_per_pixel;

        centre = point3(0,0,0);

        auto focal_length = 1.0;
        auto viewport_height = 2.0;
        auto viewport_width = viewport_height * (double(image_width)/image_height);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        auto viewport_u = vec3(viewport_width, 0, 0);
        auto viewport_v = vec3(0, -viewport_height, 0);

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left = centre
                             - vec3(0, 0, focal_length) - 
                             viewport_u/2 - viewport_v/2;
        pixe100_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }

    ray get_ray(int i,int j) const{
        auto offset =  sample_square();
        auto pixel_sample = pixe100_loc
                        + ((i + offset.x()) * pixel_delta_u)
                        + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = centre;
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin,ray_direction);
    }

    vec3 sample_square() const{
        return vec3(random_double() - 0.5,random_double() - 0.5,0);
    }

    colour ray_colour(const ray& r,int depth, const hittable& world) const{
        if(depth <= 0)
            return colour(0,0,0);
        
        hit_record rec;
        if (world.hit(r,interval(0.001,infinity),rec))
        {
            vec3 direction = rec.normal + random_unit_vector();
            return 0.5 * ray_colour(ray(rec.p,direction),depth-1,world);
        }
    
        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*colour(1.0, 1.0, 1.0) + a*colour(0.5, 0.7, 1.0);
    }
};


#endif