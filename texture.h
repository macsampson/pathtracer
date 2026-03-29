#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "interval.h"
#include "perlin.h"
#include "rtw_stb_image.h"
#include "vec3.h"
#include <memory>

class texture {

  public:
	virtual ~texture() = default;
	virtual color value(double u, double v, const point3& p) const = 0;
};

class solid_color : public texture {
  private:
	color albedo;

  public:
	solid_color(const color& albedo) : albedo(albedo) {};

	solid_color(double red, double green, double blue) : albedo(color(red, green, blue)) {}

	color value(double u, double v, const point3& p) const override {
		return albedo;
	}
};

class checker_texture : public texture {
  private:
	double inv_scale;
	std::shared_ptr<texture> even;
	std::shared_ptr<texture> odd;

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
};

class image_texture : public texture {
  private:
	rtw_image image;

  public:
	image_texture(const char* filename) : image(filename) {}

	color value(double u, double v, const point3& p) const override {
		if (image.height() <= 0)
			return color(0, 1, 1);

		u = interval(0, 1).clamp(u);
		v = 1.0 - interval(0, 1).clamp(v);

		auto i = int(u * image.width());
		auto j = int(v * image.height());
		auto pixel = image.pixel_data(i, j);

		auto color_scale = 1.0 / 255.0;
		return color(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
	}
};

class noise_texture : public texture {
  private:
	perlin noise;

  public:
	noise_texture() {}

	color value(double u, double v, const point3& p) const override {
		return color(1, 1, 1) * noise.noise(p);
	}
};

#endif
