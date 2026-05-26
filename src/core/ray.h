#ifndef RAY_H
#define RAY_H

#include "core/vec3.h"

class ray {
  private:
	point3 orig;
	vec3 dir;
	double tm;

  public:
	vec3 inv_dir;

	// Default constructor; leaves origin and direction uninitialized.
	ray() {}

	// Constructs a ray from an origin point and a direction vector.
	ray(const point3& origin, const vec3& direction, double time)
		: orig(origin), dir(direction), inv_dir(1.0 / direction.x(), 1.0 / direction.y(), 1.0 / direction.z()), tm(time) {}

	ray(const point3& origin, const vec3& direction) : ray(origin, direction, 0) {}

	// Returns the ray's origin point.
	const point3& origin() const {
		return orig;
	}
	// Returns the ray's direction vector.
	const vec3& direction() const {
		return dir;
	}

	double time() const {
		return tm;
	}

	// Returns the point along the ray at parameter t: P(t) = origin + t * direction.
	point3 at(double t) const {
		return orig + t * dir;
	}
};

#endif
