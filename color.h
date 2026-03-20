#ifndef COLOR_H
#define COLOR_H

#include "interval.h"
#include "vec3.h"

#include <algorithm>

using color = vec3;

inline void write_color(std::ostream &out, const color &pixel_color) {
	auto r = pixel_color.x();
	auto g = pixel_color.y();
	auto b = pixel_color.z();

	static const interval intesity(0.000, 0.999);
	int rbyte = int(256 * intesity.clamp(r));
	int gbyte = int(256 * intesity.clamp(g));
	int bbyte = int(256 * intesity.clamp(b));

	out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}

#endif
