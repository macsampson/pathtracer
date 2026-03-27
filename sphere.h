#ifndef SPHERE_H
#define SPHERE_H

#include "aabb.h"
#include "hittable.h"
#include "interval.h"
#include "vec3.h"
#include <memory>

class sphere : public hittable {
  private:
	ray center;
	double radius;
	std::shared_ptr<material> mat;
	aabb bbox;

  public:
	// Constructs a stationary sphere with the given center and radius (clamped to
	// non-negative).
	sphere(const point3& static_center, double radius, shared_ptr<material> mat)
		: center(static_center, vec3(0, 0, 0)), radius(std::fmax(0, radius)), mat(mat) {
		auto rvec = vec3(radius, radius, radius);
		bbox = aabb(static_center - rvec, static_center + rvec);
	}

	// Constructs a moving sphere along a ray directon
	sphere(const point3& center1, const point3& center2, double radius,
		   shared_ptr<material> mat)
		: center(center1, center2 - center1), radius(std::fmax(0, radius)), mat(mat) {
		auto rvec = vec3(radius, radius, radius);
		aabb box1(center.at(0) - rvec, center.at(0) + rvec);
		aabb box2(center.at(1) - rvec, center.at(1) + rvec);
		bbox = aabb(box1, box2);
	}

	// Tests whether the ray r hits this sphere within interval ray_t using the quadratic
	// formula. Populates rec with the hit details for the nearest valid intersection and
	// returns true.
	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		point3 current_center = center.at(r.time());
		vec3 oc = current_center - r.origin();
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
		vec3 outward_normal = (rec.point - current_center) / radius;
		rec.set_face_normal(r, outward_normal);
		rec.mat = mat;

		return true;
	}

	aabb bounding_box() const override {
		return bbox;
	}
};

#endif
