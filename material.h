#ifndef MATERIAL_H
#define MATERIAL_H

#include "hittable.h"
#include "rtweekend.h"
#include "vec3.h"
#include <cmath>

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

class dielectric : public material {

  private:
	double refraction_index;

	static double reflectance(double cosine, double refraction_index) {
		auto r0 = (1 - refraction_index) / (1 + refraction_index);
		r0 = r0 * r0;
		return r0 + (1 - r0) * std::pow((1 - cosine), 5);
	}

  public:
	dielectric(double refraction_index) : refraction_index(refraction_index) {}

	bool scatter(const ray& r_in, const hit_record& rec, color& attenuation,
				 ray& scattered) const override {
		attenuation = color(1.0, 1.0, 1.0);
		// check if the ray is entering the object or is already inside?
		double ri = rec.front_face ? (1.0 / refraction_index) : refraction_index;

		vec3 unit_direction = unit_vector(r_in.direction());
		// vec3 refracted = refract(unit_direction, rec.normal, ri); // Alwats refracts - breaks
		// Snell's Law
		double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
		double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

		bool cannot_refract = ri * sin_theta > 1.0;
		vec3 direction;

		if (cannot_refract) {
			direction = reflect(unit_direction, rec.normal);
		} else {
			direction = refract(unit_direction, rec.normal, ri);
		}

		scattered = ray(rec.point, direction);
		return true;
	}
};

#endif
