#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class ray {
  private:
	point3 orig;
	vec3 dir;

  public:
	// Default constructor; leaves origin and direction uninitialized.
	ray() {}

	// Constructs a ray from an origin point and a direction vector.
	ray(const point3& origin, const vec3& direction) : orig(origin), dir(direction) {}

	// Returns the ray's origin point.
	const point3& origin() const { return orig; }
	// Returns the ray's direction vector.
	const vec3& direction() const { return dir; }

	// Returns the point along the ray at parameter t: P(t) = origin + t * direction.
	point3 at(double t) const { return orig + t * dir; }
};

#endif
