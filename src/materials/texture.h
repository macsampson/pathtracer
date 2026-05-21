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
	image_texture(const char* filename) : image(filename) {}

	color value(double u, double v, const point3& p) const override {
		if (image.height() <= 0)
			return color(0, 1, 1);

		// Wrap UVs with fmod so values outside [0,1] tile correctly.
		// Clamping would stretch the edge pixel across any UV island that
		// extends past the boundary, causing smearing and wrong atlas lookups.
		u = u - std::floor(u);
		v = 1.0 - (v - std::floor(v));

		auto i = int(u * image.width());
		auto j = int(v * image.height());
		auto pixel = image.pixel_data(i, j);

		auto color_scale = 1.0 / 255.0;
		return color(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
	}

  private:
	rtw_image image;
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
