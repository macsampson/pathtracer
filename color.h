#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"

#include <algorithm>

using color = vec3;

inline void write_color(std::ostream &out, const color &pixel_color) {
	auto r = pixel_color.x();
	auto g = pixel_color.y();
	auto b = pixel_color.z();

	int rbyte = int(255.999 * std::clamp(r, 0.0, 1.0));
	int gbyte = int(255.999 * std::clamp(g, 0.0, 1.0));
	int bbyte = int(255.999 * std::clamp(b, 0.0, 1.0));

	out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}

#endif
