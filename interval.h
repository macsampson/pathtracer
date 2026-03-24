#ifndef INTERVAL_H
#define INTERVAL_H

#include "rtweekend.h"
#include <cmath>

class interval {
  public:
	double min, max;

	// Default constructor creates an empty interval (min > max).
	interval() : min(+infinity), max(-infinity) {}

	// Constructs an interval with the given min and max bounds.
	interval(double min, double max) : min(min), max(max) {}

	// Returns the size of the interval (max - min).
	double size() const { return max - min; }

	// Returns true if x is within [min, max] (inclusive).
	bool contains(double x) const { return min <= x && x <= max; }

	// Returns true if x is strictly within (min, max) (exclusive).
	bool surrounds(double x) const { return min < x && x < max; }

	// Clamps x to the interval [min, max].
	double clamp(double x) const {
		if (x < min)
			return min;
		if (x > max)
			return max;
		return x;
	}

	static const interval empty, universe;
};

const interval interval::empty = interval(+infinity, -infinity);
const interval interval::universe = interval(-infinity, +infinity);

#endif
