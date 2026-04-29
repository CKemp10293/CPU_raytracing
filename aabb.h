#ifndef AABB_H
#define AABB_H

#include "interval.h"
#include "ray.h"

class aabb {
public:
    interval x, y, z;

    aabb() {}
    aabb(const interval& x, const interval& y, const interval& z) : x(x), y(y), z(z) {}
    aabb(const point3& a, const point3& b) {
        x = (a[0] <= b[0]) ? interval(a[0], b[0]) : interval(b[0], a[0]);
        y = (a[1] <= b[1]) ? interval(a[1], b[1]) : interval(b[1], a[1]);
        z = (a[2] <= b[2]) ? interval(a[2], b[2]) : interval(b[2], a[2]);
    }

    const interval& axis_interval(int n) const {
        if (n == 1) return y;
        if (n == 2) return z;
        return x;
    }

    bool hit(const ray& r, interval ray_t) const {
        for (int axis = 0; axis < 3; axis++) {
            const interval& ax = axis_interval(axis);
            const double inv_d = 1.0 / r.direction()[axis];
            auto t0 = (ax.min - r.origin()[axis]) * inv_d;
            auto t1 = (ax.max - r.origin()[axis]) * inv_d;
            if (inv_d < 0.0) std::swap(t0, t1);
            if (t0 > ray_t.min) ray_t.min = t0;
            if (t1 < ray_t.max) ray_t.max = t1;
            if (ray_t.max <= ray_t.min) return false;
        }
        return true;
    }

    static aabb merge(const aabb& a, const aabb& b) {
        return aabb(
            interval(std::fmin(a.x.min, b.x.min), std::fmax(a.x.max, b.x.max)),
            interval(std::fmin(a.y.min, b.y.min), std::fmax(a.y.max, b.y.max)),
            interval(std::fmin(a.z.min, b.z.min), std::fmax(a.z.max, b.z.max))
        );
    }
};

#endif
