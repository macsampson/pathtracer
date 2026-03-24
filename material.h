#ifndef MATERIAL_H
#define MATERIAL_H

#include "hittable.h"
#include "rtweekend.h"
#include "vec3.h"

class material {
  public:
	virtual ~material() = default;

	// Scatters the incoming ray at a hit point; sets attenuation and the scattered ray.
	// Returns true if the ray is scattered, false if it is absorbed. Base class absorbs all
	// rays.
	virtual bool scatter(const ray& r_in, const hit_record& rec, color& attenuation,
						 ray& scattered) const {
		return false;
	}
};

class lambertian : public material {
  private:
	color albedo;

  public:
	// Constructs a Lambertian (diffuse) material with the given albedo color.
	lambertian(const color& albedo) : albedo(albedo) {}

	// Scatters the ray diffusely using a Lambertian distribution (normal + random unit vector).
	// Corrects near-zero scatter directions to avoid degenerate rays.
	bool scatter(const ray& r_in, const hit_record& rec, color& attenuation,
				 ray& scattered) const override {
		auto scatter_direction = rec.normal + random_unit_vector();

		if (scatter_direction.near_zero())
			scatter_direction = rec.normal;

		scattered = ray(rec.point, scatter_direction);
		attenuation = albedo;
		return true;
	}
};

class metal : public material {
  private:
	color albedo;
	double fuzz;

  public:
	// Constructs a metallic material with the given albedo color.
	metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

	// Scatters the ray via specular reflection about the surface normal.
	bool scatter(const ray& r_in, const hit_record& rec, color& attenuation,
				 ray& scattered) const override {
		vec3 reflected = reflect(r_in.direction(), rec.normal);
		reflected = unit_vector(reflected) + (fuzz * random_unit_vector());
		scattered = ray(rec.point, reflected);
		attenuation = albedo;
		return (dot(scattered.direction(), rec.normal) > 0);
	}
};

#endif
