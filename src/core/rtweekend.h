#ifndef RTWEEKEND_H
#define RTWEEKEND_H

#include <cmath>
#include <cstdint>
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

// xoshiro256+ — fast, high-quality RNG for floating-point use.
// Each thread gets its own state seeded from std::random_device.
namespace detail {
inline uint64_t rotl(uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}
struct xoshiro256p {
	uint64_t s[4];
	xoshiro256p() {
		std::random_device rd;
		for (auto& v : s)
			v = (uint64_t(rd()) << 32) | rd();
	}
	uint64_t next() {
		const uint64_t result = s[0] + s[3];
		const uint64_t t = s[1] << 17;
		s[2] ^= s[0];
		s[3] ^= s[1];
		s[1] ^= s[2];
		s[0] ^= s[3];
		s[2] ^= t;
		s[3] = rotl(s[3], 45);
		return result;
	}
};
} // namespace detail

// Returns a random double in [0, 1). Thread-safe via thread_local state.
inline double random_double() {
	thread_local detail::xoshiro256p rng;
	// Extract 53 mantissa bits for a double in [0, 1).
	return (rng.next() >> 11) * (1.0 / double(UINT64_C(1) << 53));
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
