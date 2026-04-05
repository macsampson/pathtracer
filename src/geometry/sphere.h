#ifndef SPHERE_H
#define SPHERE_H

#include "core/interval.h"
#include "core/rtweekend.h"
#include "core/vec3.h"
#include "geometry/aabb.h"
#include "geometry/hittable.h"
#include <cmath>
#include <csetjmp>
#include <memory>

class sphere : public hittable {
  private:
	ray center;
	double radius;
	std::shared_ptr<material> mat;
	aabb bbox;

	static void get_sphere_uv(const point3& p, double& u, double& v) {
		auto theta = std::acos(-p.y());
		auto phi = std::atan2(-p.z(), p.x()) + pi;

		u = phi / (2 * pi);
		v = theta / pi;
	}

  public:
	// Constructs a STATIONARY sphere with the given center and radius (clamped to
	// non-negative).
	sphere(const point3& static_center, double radius, shared_ptr<material> mat)
		: center(static_center, vec3(0, 0, 0)), radius(std::fmax(0, radius)), mat(mat) {
		auto rvec = vec3(radius, radius, radius);
		bbox = aabb(static_center - rvec, static_center + rvec);
	}

	// Constructs a MOVING sphere along a ray directon
	sphere(const point3& center1, const point3& center2, double radius, shared_ptr<material> mat)
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
		get_sphere_uv(outward_normal, rec.u, rec.v);
		rec.mat = mat;

		return true;
	}

	vec3 random_point(const point3& origin) const override {
		vec3 to_center = center.at(0) - origin; // for stationary; use time for moving
		double d = to_center.length();

		// If we're inside the sphere, fall back to uniform full-sphere sampling
		if (d < radius)
			return center.at(0) + radius * random_unit_vector();

		double cos_theta_max = std::sqrt(1.0 - (radius * radius) / (d * d));

		// Sample a random direction within the cone
		// Uniform cone sampling: cos(theta) = 1 + r*(cos_theta_max - 1)
		double r1 = random_double();
		double r2 = random_double();
		double cos_theta = 1.0 + r1 * (cos_theta_max - 1.0);
		double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);
		double phi = 2.0 * pi * r2;

		// Build a local coordinate frame where z points toward the sphere center
		vec3 w = unit_vector(to_center);
		vec3 a = (std::fabs(w.x()) > 0.9) ? vec3(0, 1, 0) : vec3(1, 0, 0);
		vec3 v = unit_vector(cross(w, a));
		vec3 u = cross(w, v);

		// Convert from spherical to Cartesian in this local frame
		vec3 dir = sin_theta * std::cos(phi) * u + sin_theta * std::sin(phi) * v + cos_theta * w;

		// Return the actual point on the sphere surface by tracing to it
		// (or just return origin + dir and let the caller use it as a direction)
		hit_record rec;
		if (this->hit(ray(origin, dir), interval(0.001, infinity), rec))
			return rec.point;

		// Fallback (shouldn't happen if math is right)
		return center.at(0) + radius * random_unit_vector();
	}

	double pdf_value(const point3& origin, const vec3& dir) const override {
		// Does this direction hit the sphere at all?
		hit_record rec;
		if (!this->hit(ray(origin, dir), interval(0.001, infinity), rec))
			return 0;

		vec3 to_center = center.at(0) - origin;
		double d_sq = to_center.length_squared();

		double cos_theta_max
			= (d_sq < radius * radius) ? 1.0 / (4.0 * pi) : std::sqrt(1.0 - (radius * radius) / d_sq);
		double solid_angle = 2.0 * pi * (1.0 - cos_theta_max);

		return 1.0 / solid_angle;
	}

	aabb bounding_box() const override {
		return bbox;
	}
};

#endif
