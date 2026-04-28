#ifndef RAYTRACE_H
#define RAYTRACE_H

#include <cmath>
#include <iostream>
#include <cstdlib>
#include <limits>
#include <memory>

// c++ std usings

using std::make_shared;
using std::shared_ptr;

// consts
const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

// Util
inline double degrees_to_radians(double degrees){
    return degrees * pi / 180;
}

inline double random_double(){
    // returns a random real in [0,1]
    return std::rand() / (RAND_MAX + 1.0);
}

inline double random_double(double min,double max){
    return min + (max-min)*random_double();
}

// common headers

#include "colour.h"
#include "ray.h"
#include "vec3.h"
#include "interval.h"

#endif