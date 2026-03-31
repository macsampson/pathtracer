#ifndef BVH_H
#define BVH_H

#include "geometry/aabb.h"
#include "core/interval.h"
#include "geometry/hittable.h"
#include "geometry/hittable_list.h"
#include <algorithm>
#include <memory>

class bvh_node : public hittable {
  public:
	bvh_node(hittable_list list) : bvh_node(list.objects, 0, list.objects.size()) {}
	bvh_node(std::vector<std::shared_ptr<hittable>>& objects, size_t start, size_t end) {
		// Build the enclosing AABB for all objects in this slice first.
		// This is used both as this node's bbox and to pick the best split axis.
		bbox = aabb::empty;
		for (size_t object_index = start; object_index < end; object_index++) {
			bbox = aabb(bbox, objects[object_index]->bounding_box());
		}

		// Split along the longest axis to produce the most balanced tree.
		// Splitting where objects are most spread out keeps subtree bboxes small,
		// which maximises the chance of early rejection during traversal.
		// (Alternative: random axis — simpler but produces worse trees on average.)
		int axis = bbox.longest_axis();
		// int axis = random_int(0, 2);

		auto comparator = (axis == 0) ? box_x_compare : (axis == 1) ? box_y_compare : box_z_compare;

		size_t object_span = end - start;

		if (object_span == 1) {
			// Leaf node: store the single object in both children to avoid null checks.
			left = right = objects[start];
		} else if (object_span == 2) {
			left = objects[start];
			right = objects[start + 1];
		} else {
			// Sort objects spatially along the chosen axis, then split at the midpoint.
			// Spatially adjacent objects end up in the same subtree, keeping bboxes tight.
			std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);

			auto mid = start + object_span / 2;
			left = make_shared<bvh_node>(objects, start, mid);
			right = make_shared<bvh_node>(objects, mid, end);
		}
		// bbox is already computed above from all objects; no need to re-derive from children.
		// bbox = aabb(left->bounding_box(), right->bounding_box());
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		// Reject the entire subtree early if the ray misses this node's bounding box.
		if (!bbox.hit(r, ray_t))
			return false;
		bool hit_left = left->hit(r, ray_t, rec);
		// If the left child already found a hit, there's no need to check the right child
		// for anything farther away — shrink the search interval to rec.t.
		// This both prunes unnecessary work and ensures the nearest hit is always returned.
		bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

		return hit_left || hit_right;
	}

	aabb bounding_box() const override {
		return bbox;
	}

  private:
	std::shared_ptr<hittable> left;
	std::shared_ptr<hittable> right;
	aabb bbox;

	// Compare two hittables by the min bound of their AABB on a given axis.
	// Used to sort objects spatially before splitting into left/right subtrees.
	static bool box_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b, int axis_index) {
		auto a_axis_interval = a->bounding_box().axis_interval(axis_index);
		auto b_axis_interval = b->bounding_box().axis_interval(axis_index);
		return a_axis_interval.min < b_axis_interval.min;
	}

	static bool box_x_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
		return box_compare(a, b, 0);
	}
	static bool box_y_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
		return box_compare(a, b, 1);
	}
	static bool box_z_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
		return box_compare(a, b, 2);
	}
};

#endif
