#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "raytrace.h"

class sphere : public hittable {
    public:
    sphere(const point3& centre,double radius) : centre(centre), 
           radius(std::fmax(0,radius)) {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override{
        vec3 oc = centre - r.origin();
        auto a = r.direction().length_sqaured();
        auto h = dot(r.direction(), oc);
        auto c = oc.length_sqaured() - radius*radius;
        auto discriminant = h*h - a*c;

        if (discriminant < 0)
        {
            return false;
        }

        auto sqrtd = std::sqrt(discriminant);

        // find nearest root in acceptable range
        auto root = (h-sqrtd) / a;
        if (!ray_t.surrounds(root))
        {
            root = (h + sqrtd) / a;
            if (!ray_t.surrounds(root))
            {
                return false;
            }
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - centre) / radius;
        rec.set_face_normal(r,outward_normal);
        
        return true;
    }

    private:
    point3 centre;
    double radius;
};
#endif