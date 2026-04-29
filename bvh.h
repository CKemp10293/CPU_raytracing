#ifndef BVH_H
#define BVH_H

#include "aabb.h"
#include "hittable.h"
#include "hittable_list.h"
#include <algorithm>

class bvh_node : public hittable {
public:
    bvh_node(hittable_list& list)
        : bvh_node(list.objects, 0, list.objects.size()) {}

    bvh_node(std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end) {
        int axis = longest_axis(objects, start, end);

        auto cmp = [axis](const shared_ptr<hittable>& a, const shared_ptr<hittable>& b) {
            return a->bounding_box().axis_interval(axis).min <
                   b->bounding_box().axis_interval(axis).min;
        };

        size_t span = end - start;
        if (span == 1) {
            left = right = objects[start];
        } else if (span == 2) {
            left  = objects[start];
            right = objects[start + 1];
        } else {
            std::sort(objects.begin() + start, objects.begin() + end, cmp);
            size_t mid = start + span / 2;
            left  = make_shared<bvh_node>(objects, start, mid);
            right = make_shared<bvh_node>(objects, mid,   end);
        }

        bbox = aabb::merge(left->bounding_box(), right->bounding_box());
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!bbox.hit(r, ray_t)) return false;
        bool hit_left  = left->hit(r, ray_t, rec);
        bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);
        return hit_left || hit_right;
    }

    aabb bounding_box() const override { return bbox; }

private:
    shared_ptr<hittable> left, right;
    aabb bbox;

    static int longest_axis(std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end) {
        aabb combined;
        for (size_t i = start; i < end; i++)
            combined = aabb::merge(combined, objects[i]->bounding_box());
        double dx = combined.x.size();
        double dy = combined.y.size();
        double dz = combined.z.size();
        if (dx > dy && dx > dz) return 0;
        if (dy > dz) return 1;
        return 2;
    }
};

#endif
