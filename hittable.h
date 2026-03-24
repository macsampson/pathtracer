#ifndef HITTABLE_H
#define HITTABLE_H

#include "rtweekend.h"
#include <memory>

using std::shared_ptr;

class material;

class hit_record {
  public:
	point3 point;
	vec3 normal;
	double t;
	bool front_face;
	shared_ptr<material> mat;

	// Sets the hit record normal vector so it always points against the incoming ray.
	// Determines front_face based on the ray direction vs. the outward normal.
	// NOTE: the parameter `outward_normal` is assumed to have unit length
	void set_face_normal(const ray& r, const vec3& outward_normal) {
		front_face = dot(r.direction(), outward_normal) < 0;
		normal = front_face ? outward_normal : -outward_normal;
	}
};

class hittable {
  public:
	virtual ~hittable() = default;
	// Tests whether the ray r hits this object within the interval ray_t.
	// If a hit is found, populates rec with hit details and returns true.
	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
};

#endif
