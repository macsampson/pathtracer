#ifndef AABB_H
#define AABB_H

#include "interval.h"
#include "vec3.h"

class aabb {
  public:
	interval x, y, z;

	aabb() {}; // AABB is empty by default

	// Construct from three explicit axis-aligned intervals.
	aabb(const interval& x, const interval& y, const interval& z) : x(x), y(y), z(z) {
		pad_to_minimums();
	}

	// Construct the AABB tightly enclosing two corner points.
	// Points can be in any order; each axis is sorted independently.
	aabb(const point3& a, const point3& b) {
		x = (a[0] <= b[0]) ? interval(a[0], b[0]) : interval(b[0], a[0]);
		y = (a[1] <= b[1]) ? interval(a[1], b[1]) : interval(b[1], a[1]);
		z = (a[2] <= b[2]) ? interval(a[2], b[2]) : interval(b[2], a[2]);
	}

	// Construct the AABB tightly enclosing two existing AABBs.
	aabb(const aabb& box0, const aabb& box1) {
		x = interval(box0.x, box1.x);
		y = interval(box0.y, box1.y);
		z = interval(box0.z, box1.z);
	}

	const interval& axis_interval(int n) const {
		if (n == 1)
			return y;
		if (n == 2)
			return z;
		return x;
	}

	// Slab method: a ray hits the box if there is a time interval where it is
	// simultaneously inside all three axis-aligned slabs (pairs of infinite planes).
	// Each slab contributes a [t0, t1] crossing interval; we intersect all three.
	// If the result is non-empty, the ray passes through the box.
	bool hit(const ray& r, interval ray_t) const {
		const point3& ray_orig = r.origin();
		const vec3& ray_dir = r.direction();

		for (int axis = 0; axis < 3; axis++) {
			const interval& ax = axis_interval(axis);
			// Precompute reciprocal once per axis (cheaper than two divisions).
			// Sign is preserved, so negative direction correctly swaps t0/t1 below.
			const double adinv = 1.0 / ray_dir[axis];

			// Solve P(t) = origin + t*direction for t at each slab boundary plane.
			auto t0 = (ax.min - ray_orig[axis]) * adinv;
			auto t1 = (ax.max - ray_orig[axis]) * adinv;

			// For a negative direction component, the ray hits ax.max first, so t0 > t1.
			// In both branches we clamp ray_t to [entry, exit] for this slab.
			if (t0 < t1) {
				if (t0 > ray_t.min)
					ray_t.min = t0;
				if (t1 < ray_t.max)
					ray_t.max = t1;
			} else {
				if (t1 > ray_t.min)
					ray_t.min = t1;
				if (t0 < ray_t.max)
					ray_t.max = t0;
			}

			// If the accumulated interval has collapsed, the ray missed the box.
			if (ray_t.max <= ray_t.min)
				return false;
		}
		return true;
	}

	// Returns the index of the longest axis (0=x, 1=y, 2=z).
	// Used by the BVH to choose the best axis to split objects along.
	int longest_axis() const {
		if (x.size() > y.size())
			return x.size() > z.size() ? 0 : 2;
		else
			return y.size() > z.size() ? 1 : 2;
	}

	static const aabb empty, universe;

  private:
	void pad_to_minimums() {
		double delta = 0.0001;

		if (x.size() < delta)
			x = x.expand(delta);
		if (y.size() < delta)
			y = y.expand(delta);
		if (z.size() < delta)
			z = z.expand(delta);
	}
};

const aabb aabb::empty = aabb(interval::empty, interval::empty, interval::empty);
const aabb aabb::universe = aabb(interval::universe, interval::universe, interval::universe);

aabb operator+(const aabb& bbox, const vec3& offset) {
	return aabb(bbox.x + offset.x(), bbox.y + offset.y(), bbox.z + offset.z());
}

aabb operator+(const vec3& offset, const aabb& bbox) {
	return bbox + offset;
}

#endif
