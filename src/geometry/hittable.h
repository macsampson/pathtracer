#ifndef HITTABLE_H
#define HITTABLE_H

#include "core/interval.h"
#include "core/rtweekend.h"
#include "core/vec3.h"
#include "geometry/aabb.h"
#include <cmath>
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
	double u;
	double v;

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

	// returns a random point on the surface of a hittable
	virtual vec3 random_point(const point3& origin) const {
		return vec3(0, 0, 0);
	};

	// returns the pdf for a sampling direction from origin
	virtual double pdf_value(const point3& origin, const vec3& dir) const {
		return 0;
	};

	virtual aabb bounding_box() const = 0;
};

// Wraps a hittable and shifts it by a fixed offset in world space.
// Instead of moving the object, we move the ray in the opposite direction,
// test the object at its original position, then shift the hit point back.
class translate : public hittable {
  public:
	translate(shared_ptr<hittable> object, const vec3& offset) : object(object), offset(offset) {
		bbox = object->bounding_box() + offset;
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		// Move the ray backward by offset — equivalent to moving the object forward.
		ray offset_r(r.origin() - offset, r.direction(), r.time());

		if (!object->hit(offset_r, ray_t, rec))
			return false;

		// Shift the hit point back into world space.
		// The normal doesn't need adjusting — translation doesn't change orientation.
		rec.point += offset;

		return true;
	}

	aabb bounding_box() const override {
		return bbox;
	}

  private:
	shared_ptr<hittable> object;
	vec3 offset;
	aabb bbox;
};

// Wraps a hittable and rotates it around the Y axis by a fixed angle.
// Uses the same "transform the ray, not the object" approach as translate:
// rotate the ray into object space, test, then rotate the result back to world space.
class rotate_y : public hittable {
  public:
	rotate_y(shared_ptr<hittable> object, double angle) : object(object) {
		auto radians = degrees_to_radians(angle);
		sin_theta = std::sin(radians);
		cos_theta = std::cos(radians);
		bbox = object->bounding_box();

		// Compute the new AABB by rotating all 8 corners of the original AABB
		// and taking the min/max extents of the rotated corners.
		// A rotated box is no longer axis-aligned, so we can't just rotate the intervals.
		point3 min(infinity, infinity, infinity);
		point3 max(-infinity, -infinity, -infinity);

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				for (int k = 0; k < 2; k++) {
					// Select each of the 8 corners using i/j/k as min/max selectors.
					auto x = i * bbox.x.max + (1 - i) * bbox.x.min;
					auto y = j * bbox.y.max + (1 - j) * bbox.y.min;
					auto z = k * bbox.z.max + (1 - k) * bbox.z.min;

					// Apply Y-axis rotation matrix to the corner. Y is unchanged.
					auto newx = cos_theta * x + sin_theta * z;
					auto newz = -sin_theta * x + cos_theta * z;

					vec3 rotated_corner(newx, y, newz);

					for (int c = 0; c < 3; c++) {
						min[c] = std::fmin(min[c], rotated_corner[c]);
						max[c] = std::fmax(max[c], rotated_corner[c]);
					}
				}
			}
		}
		bbox = aabb(min, max);
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		// Rotate the ray into object space (inverse rotation = negate the angle).
		// Inverse of rotation by theta is rotation by -theta: cos is unchanged, sin flips.
		auto origin = point3((cos_theta * r.origin().x()) - (sin_theta * r.origin().z()), r.origin().y(),
							 (sin_theta * r.origin().x()) + (cos_theta * r.origin().z()));

		auto direction
			= vec3((cos_theta * r.direction().x()) - (sin_theta * r.direction().z()), r.direction().y(),
				   (sin_theta * r.direction().x()) + (cos_theta * r.direction().z()));

		ray rotated_ray(origin, direction, r.time());

		if (!object->hit(rotated_ray, ray_t, rec))
			return false;

		// Rotate the hit point and normal back to world space (forward rotation).
		rec.point = point3((cos_theta * rec.point.x()) + (sin_theta * rec.point.z()), rec.point.y(),
						   (-sin_theta * rec.point.x()) + (cos_theta * rec.point.z()));

		rec.normal = vec3((cos_theta * rec.normal.x()) + (sin_theta * rec.normal.z()), rec.normal.y(),
						  (-sin_theta * rec.normal.x()) + (cos_theta * rec.normal.z()));

		return true;
	}

	aabb bounding_box() const override {
		return bbox;
	}

  private:
	shared_ptr<hittable> object;
	double cos_theta;
	double sin_theta;
	aabb bbox;
};

// Rotation around the X axis by `angle` degrees.
// Forward rotation: y' = cos·y − sin·z,  z' = sin·y + cos·z,  x unchanged.
// Ray is transformed with the inverse (negate sin) into object space;
// hit point and normal are rotated forward back to world space.
class rotate_x : public hittable {
  public:
	rotate_x(shared_ptr<hittable> object, double angle) : object(object) {
		auto radians = degrees_to_radians(angle);
		sin_theta = std::sin(radians);
		cos_theta = std::cos(radians);
		bbox = object->bounding_box();

		// Rotate all 8 AABB corners and re-fit — a rotated box is no longer axis-aligned.
		point3 min(infinity, infinity, infinity);
		point3 max(-infinity, -infinity, -infinity);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				for (int k = 0; k < 2; k++) {
					auto x = i * bbox.x.max + (1 - i) * bbox.x.min;
					auto y = j * bbox.y.max + (1 - j) * bbox.y.min;
					auto z = k * bbox.z.max + (1 - k) * bbox.z.min;
					auto newy = cos_theta * y - sin_theta * z;
					auto newz = sin_theta * y + cos_theta * z;
					vec3 corner(x, newy, newz);
					for (int c = 0; c < 3; c++) {
						min[c] = std::fmin(min[c], corner[c]);
						max[c] = std::fmax(max[c], corner[c]);
					}
				}
			}
		}
		bbox = aabb(min, max);
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		// Inverse rotation (negate sin) transforms the ray into object space.
		auto origin    = point3(r.origin().x(),
		                        cos_theta * r.origin().y()    + sin_theta * r.origin().z(),
		                        -sin_theta * r.origin().y()   + cos_theta * r.origin().z());
		auto direction = vec3(r.direction().x(),
		                      cos_theta * r.direction().y()   + sin_theta * r.direction().z(),
		                      -sin_theta * r.direction().y()  + cos_theta * r.direction().z());
		ray rotated_r(origin, direction, r.time());
		if (!object->hit(rotated_r, ray_t, rec))
			return false;
		// Forward rotation brings hit point and normal back to world space.
		rec.point  = point3(rec.point.x(),
		                     cos_theta * rec.point.y()  - sin_theta * rec.point.z(),
		                     sin_theta * rec.point.y()  + cos_theta * rec.point.z());
		rec.normal = vec3(rec.normal.x(),
		                   cos_theta * rec.normal.y() - sin_theta * rec.normal.z(),
		                   sin_theta * rec.normal.y() + cos_theta * rec.normal.z());
		return true;
	}

	aabb bounding_box() const override { return bbox; }

  private:
	shared_ptr<hittable> object;
	double cos_theta, sin_theta;
	aabb bbox;
};

// Rotation around the Z axis by `angle` degrees.
// Forward rotation: x' = cos·x − sin·y,  y' = sin·x + cos·y,  z unchanged.
class rotate_z : public hittable {
  public:
	rotate_z(shared_ptr<hittable> object, double angle) : object(object) {
		auto radians = degrees_to_radians(angle);
		sin_theta = std::sin(radians);
		cos_theta = std::cos(radians);
		bbox = object->bounding_box();

		point3 min(infinity, infinity, infinity);
		point3 max(-infinity, -infinity, -infinity);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				for (int k = 0; k < 2; k++) {
					auto x = i * bbox.x.max + (1 - i) * bbox.x.min;
					auto y = j * bbox.y.max + (1 - j) * bbox.y.min;
					auto z = k * bbox.z.max + (1 - k) * bbox.z.min;
					auto newx = cos_theta * x - sin_theta * y;
					auto newy = sin_theta * x + cos_theta * y;
					vec3 corner(newx, newy, z);
					for (int c = 0; c < 3; c++) {
						min[c] = std::fmin(min[c], corner[c]);
						max[c] = std::fmax(max[c], corner[c]);
					}
				}
			}
		}
		bbox = aabb(min, max);
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		auto origin    = point3(cos_theta * r.origin().x()    + sin_theta * r.origin().y(),
		                        -sin_theta * r.origin().x()   + cos_theta * r.origin().y(),
		                        r.origin().z());
		auto direction = vec3(cos_theta * r.direction().x()   + sin_theta * r.direction().y(),
		                      -sin_theta * r.direction().x()  + cos_theta * r.direction().y(),
		                      r.direction().z());
		ray rotated_r(origin, direction, r.time());
		if (!object->hit(rotated_r, ray_t, rec))
			return false;
		rec.point  = point3(cos_theta * rec.point.x()  - sin_theta * rec.point.y(),
		                     sin_theta * rec.point.x()  + cos_theta * rec.point.y(),
		                     rec.point.z());
		rec.normal = vec3(cos_theta * rec.normal.x() - sin_theta * rec.normal.y(),
		                   sin_theta * rec.normal.x() + cos_theta * rec.normal.y(),
		                   rec.normal.z());
		return true;
	}

	aabb bounding_box() const override { return bbox; }

  private:
	shared_ptr<hittable> object;
	double cos_theta, sin_theta;
	aabb bbox;
};

class scale : public hittable {
  public:
	scale(shared_ptr<hittable> object, double s) : object(object), s(s) {
		auto b = object->bounding_box();
		bbox = aabb(interval(b.x.min * s, b.x.max * s), interval(b.y.min * s, b.y.max * s),
					interval(b.z.min * s, b.z.max * s));
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		ray scaled_r(r.origin() / s, r.direction() / s, r.time());
		if (!object->hit(scaled_r, ray_t, rec))
			return false;
		rec.point *= s;
		return true;
	}

	aabb bounding_box() const override {
		return bbox;
	}

  private:
	shared_ptr<hittable> object;
	double s;
	aabb bbox;
};

#endif
