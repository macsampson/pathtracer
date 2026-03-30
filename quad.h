#ifndef QUAD_H
#define QUAD_H

#include "aabb.h"
#include "hittable.h"
#include "hittable_list.h"
#include "interval.h"
#include "ray.h"
#include "vec3.h"
#include <cmath>
#include <memory>
// A parallelogram defined by a corner point Q and two edge vectors u and v.
// Any point on the quad's plane can be expressed as Q + alpha*u + beta*v.
// The quad is the region where alpha and beta are both in [0, 1].
class quad : public hittable {
  public:
	// Q: corner point. u, v: edge vectors from Q defining the parallelogram.
	quad(const point3& Q, const vec3& u, const vec3& v, std::shared_ptr<material> mat)
		: Q(Q), u(u), v(v), mat(mat) {
		auto n = cross(u, v);
		normal = unit_vector(n);
		// D is the plane constant in the equation dot(normal, p) = D.
		// Since Q lies on the plane, D = dot(normal, Q).
		D = dot(normal, Q);
		// w is used to recover alpha/beta from a hit point (see hit()).
		// w = n / dot(n, n) ensures dot(w, cross(p, v)) extracts the alpha component.
		w = n / dot(n, n);

		set_bounding_box();
	}

	// Builds the AABB from both diagonals of the quad to handle axis-aligned quads
	// (which would be degenerate if built from a single diagonal). aabb's pad_to_minimums
	// handles any remaining zero-thickness axes.
	virtual void set_bounding_box() {
		auto bbox_diagonal1 = aabb(Q, Q + u + v);
		auto bbox_diagonal2 = aabb(Q + u, Q + v);
		bbox = aabb(bbox_diagonal1, bbox_diagonal2);
	}

	aabb bounding_box() const override {
		return bbox;
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		// denom = dot(normal, direction). If near zero the ray is parallel to the plane — no hit.
		auto denom = dot(normal, r.direction());
		if (std::fabs(denom) < 1e-8)
			return false;

		// Solve dot(normal, r.at(t)) = D for t — the ray's intersection with the infinite plane.
		auto t = (D - dot(normal, r.origin())) / denom;

		if (!ray_t.contains(t))
			return false;

		// Express the hit point relative to Q as alpha*u + beta*v.
		// alpha and beta are the parallelogram coordinates of the hit point.
		// w is constructed so that dot(w, cross(p, v)) = alpha and dot(w, cross(u, p)) = beta.
		auto intersection = r.at(t);
		vec3 planar_hitpt_vector = intersection - Q;
		auto alpha = dot(w, cross(planar_hitpt_vector, v));
		auto beta = dot(w, cross(u, planar_hitpt_vector));

		// Reject if the hit point is outside the [0,1]x[0,1] quad region.
		if (!is_interior(alpha, beta, rec))
			return false;

		rec.t = t;
		rec.point = intersection;
		rec.mat = mat;
		rec.set_face_normal(r, normal);

		return true;
	}

	// Returns true if (a, b) is inside the unit square [0,1]x[0,1].
	// Also writes the uv texture coordinates into rec.
	// Virtual so subclasses (e.g. disks, triangles) can override the interior test.
	virtual bool is_interior(double a, double b, hit_record& rec) const {
		interval unit_interval = interval(0, 1);

		if (!unit_interval.contains(a) || !unit_interval.contains(b))
			return false;

		rec.u = a;
		rec.v = b;
		return true;
	}

  private:
	point3 Q;  // Corner of the parallelogram
	vec3 u, v; // Edge vectors from Q
	vec3 w;	   // Helper vector for recovering alpha/beta from a hit point
	shared_ptr<material> mat;
	aabb bbox;
	vec3 normal; // Unit normal to the plane
	double D;	 // Plane constant: dot(normal, p) = D for any point p on the plane
};

inline shared_ptr<hittable_list> box(const point3& a, const point3& b, shared_ptr<material> mat) {
	auto sides = make_shared<hittable_list>();

	auto min = point3(std::fmin(a.x(), b.x()), std::fmin(a.y(), b.y()), std::fmin(a.z(), b.z()));
	auto max = point3(std::fmax(a.x(), b.x()), std::fmax(a.y(), b.y()), std::fmax(a.z(), b.z()));

	auto dx = vec3(max.x() - min.x(), 0, 0);
	auto dy = vec3(0, max.y() - min.y(), 0);
	auto dz = vec3(0, 0, max.z() - min.z());

	sides->add(make_shared<quad>(point3(min.x(), min.y(), max.z()), dx, dy, mat));	// front
	sides->add(make_shared<quad>(point3(max.x(), min.y(), max.z()), -dz, dy, mat)); // right
	sides->add(make_shared<quad>(point3(max.x(), min.y(), min.z()), -dx, dy, mat)); // back
	sides->add(make_shared<quad>(point3(min.x(), min.y(), min.z()), dz, dy, mat));	// left
	sides->add(make_shared<quad>(point3(min.x(), max.y(), max.z()), dx, -dz, mat)); // top
	sides->add(make_shared<quad>(point3(min.x(), min.y(), min.z()), dx, dz, mat));	// bottom

	return sides;
}

#endif
