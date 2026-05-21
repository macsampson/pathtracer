#ifndef TEXTURE_H
#define TEXTURE_H

#include "core/color.h"
#include "core/interval.h"
#include "core/vec3.h"
#include "materials/perlin.h"
#include "rendering/wrap_stb_image.h"
#include <memory>

class texture {

  public:
	virtual ~texture() = default;
	virtual color value(double u, double v, const point3& p) const = 0;
};

class solid_color : public texture {

  public:
	solid_color(const color& albedo) : albedo(albedo) {};

	solid_color(double red, double green, double blue) : albedo(color(red, green, blue)) {}

	color value(double u, double v, const point3& p) const override {
		return albedo;
	}

  private:
	color albedo;
};

class checker_texture : public texture {
  public:
	checker_texture(double scale, std::shared_ptr<texture> even, shared_ptr<texture> odd)
		: inv_scale(1.0 / scale), even(even), odd(odd) {}

	checker_texture(double scale, const color& c1, const color& c2)
		: checker_texture(scale, make_shared<solid_color>(c1), make_shared<solid_color>(c2)) {}

	color value(double u, double v, const point3& p) const override {
		auto xInteger = int(std::floor(inv_scale * p.x()));
		auto yInteger = int(std::floor(inv_scale * p.y()));
		auto zInteger = int(std::floor(inv_scale * p.z()));

		bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;

		return isEven ? even->value(u, v, p) : odd->value(u, v, p);
	}

  private:
	double inv_scale;
	std::shared_ptr<texture> even;
	std::shared_ptr<texture> odd;
};

class image_texture : public texture {
  public:
	// mirror_u: use GL_MIRRORED_REPEAT for U — tile [1,2] samples in reverse (1.3→0.7
	// instead of 0.3). Required for Blender mirror-modifier UV layouts where one half
	// of a symmetric mesh sits in the [1,2] tile. Leave false for models whose UVs sit
	// outside [0,1] for other reasons (e.g. 3ds Max negative-V offsets).
	image_texture(const char* filename, bool mirror_u = false)
	    : image(filename), mirror_u(mirror_u) {}

	color value(double u, double v, const point3& p) const override {
		if (image.height() <= 0)
			return color(0, 1, 1);

		u = wrap(u, mirror_u);
		// V always uses simple wrap before flipping — negative V offsets (3ds Max)
		// and tiny overshoots both resolve correctly this way.
		v = 1.0 - wrap(v, false);

		auto i = int(u * image.width());
		auto j = int(v * image.height());
		auto pixel = image.pixel_data(i, j);

		auto color_scale = 1.0 / 255.0;
		return color(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
	}

  private:
	// Simple wrap: x=1.3 → 0.3. Mirrored wrap: x=1.3 → 0.7 (reverses direction
	// in odd tiles). std::abs(n) guards against negative n in C++ where -1%2 == -1.
	static double wrap(double x, bool mirrored) {
		int n = (int)std::floor(x);
		double frac = x - n;
		return (mirrored && std::abs(n) % 2 == 1) ? 1.0 - frac : frac;
	}

	rtw_image image;
	bool mirror_u = false;
};

class noise_texture : public texture {

  public:
	noise_texture(double scale) : scale(scale) {}

	color value(double u, double v, const point3& p) const override {
		return color(0.5, 0.5, 0.5) * (1 + std::sin(scale * p.z() + 10 * noise.turb(p, 7)));
	}

  private:
	perlin noise;
	double scale;
};

#endif
