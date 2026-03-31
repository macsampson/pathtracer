#ifndef RTWEEKEND_H
#define RTWEEKEND_H

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <random>

using std::make_shared;
using std::shared_ptr;

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

// Converts an angle in degrees to radians.
inline double degrees_to_radians(double degrees) {
	return degrees * pi / 180.0;
}

// Returns a random double in [0, 1). Updated to use thread-safe rng
inline double random_double() {
	thread_local std::mt19937 rng{std::random_device{}()};
	thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
	return dist(rng);
}

// Returns a random double in [min, max).
inline double random_double(double min, double max) {
	return min + (max - min) * random_double();
}

inline int random_int(int min, int max) {
	return int(random_double(min, max + 1));
}

#include "core/color.h"
#include "core/interval.h"
#include "core/ray.h"
#include "core/vec3.h"

#endif
