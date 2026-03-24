#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "interval.h"
#include <memory>

class sphere : public hittable {
  private:
	point3 center;
	double radius;
	std::shared_ptr<material> mat;

  public:
	// Constructs a sphere with the given center and radius (clamped to non-negative).
	sphere(const point3& center, double radius, shared_ptr<material> mat)
		: center(center), radius(std::fmax(0, radius)), mat(mat) {}

	// Tests whether the ray r hits this sphere within interval ray_t using the quadratic
	// formula. Populates rec with the hit details for the nearest valid intersection and
	// returns true.
	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		vec3 oc = center - r.origin();
		auto a = r.direction().length_squared();
		auto h = dot(r.direction(), oc);
		auto c = oc.length_squared() - radius * radius;

		auto discriminant = h * h - a * c;
		if (discriminant < 0)
			return false;

		auto sqrtd = std::sqrt(discriminant);

		// Find the nearest root that lies in teh acceptable range.
		auto root = (h - sqrtd) / a;
		if (!ray_t.surrounds(root)) {
			root = (h + sqrtd) / a;
			if (!ray_t.surrounds(root))
				return false;
		}

		rec.t = root;
		rec.point = r.at(rec.t);
		// rec.normal = (rec.point - center) / radius;
		vec3 outward_normal = (rec.point - center) / radius;
		rec.set_face_normal(r, outward_normal);
		rec.mat = mat;

		return true;
	}
};

#endif
