#ifndef COLOR_H
#define COLOR_H

#include "core/interval.h"
#include "core/rtweekend.h"
#include "core/vec3.h"

#include <algorithm>
#include <cmath>

using color = vec3;

// Applies a gamma-2 correction by taking the square root of a linear color component.
// Returns 0 for non-positive values.
inline double linear_to_gamma(double linear_component) {
	if (linear_component > 0) {
		return std::sqrt(linear_component);
	}
	return 0;
}

// Applies gamma correction and writes a pixel's RGB values (0-255) to the output stream.
inline void write_color(std::ostream& out, const color& pixel_color) {
	auto r = pixel_color.x();
	auto g = pixel_color.y();
	auto b = pixel_color.z();

	// replace NaN with zero
	// if (r != r)
	// 	r = 0.0;
	// if (g != g)
	// 	g = 0.0;
	// if (b != b)
	// 	b = 0.0;

	// Apply a linear to gamma transform for gamma 2
	r = linear_to_gamma(r);
	g = linear_to_gamma(g);
	b = linear_to_gamma(b);

	static const interval intesity(0.000, 0.999);
	int rbyte = int(256 * intesity.clamp(r));
	int gbyte = int(256 * intesity.clamp(g));
	int bbyte = int(256 * intesity.clamp(b));

	out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}

#endif
