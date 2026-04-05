#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "core/interval.h"
#include "core/ray.h"
#include "core/rtweekend.h"
#include "geometry/aabb.h"
#include "geometry/hittable.h"

#include <vector>

class hittable_list : public hittable {
  private:
	aabb bbox;

  public:
	std::vector<shared_ptr<hittable>> objects;

	// Default constructor creates an empty list.
	hittable_list() {}
	// Constructs a list containing a single object.
	hittable_list(shared_ptr<hittable> object) {
		add(object);
	}

	// Removes all objects from the list.
	void clear() {
		objects.clear();
	}

	// Adds an object to the scene.
	void add(shared_ptr<hittable> object) {
		objects.push_back(object);
		bbox = aabb(bbox, object->bounding_box());
	}

	// Tests the ray against all objects and records the closest hit within ray_t.
	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		hit_record temp_rec;
		bool hit_anything = false;
		auto closest_so_far = ray_t.max;

		for (const auto& object : objects) {
			if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
				hit_anything = true;
				closest_so_far = temp_rec.t;
				rec = temp_rec;
			}
		}
		return hit_anything;
	}

	vec3 random_point(const point3& origin) const override {
		// Pick a random light uniformly
		int idx = int(random_double() * objects.size());
		idx = std::min(idx, (int)objects.size() - 1);
		return objects[idx]->random_point(origin);
	}

	double pdf_value(const point3& origin, const vec3& dir) const override {
		// Average PDF across all lights (mixture PDF)
		double sum = 0;
		for (const auto& obj : objects)
			sum += obj->pdf_value(origin, dir);
		return sum / objects.size();
	}

	aabb bounding_box() const override {
		return bbox;
	}
};

#endif
