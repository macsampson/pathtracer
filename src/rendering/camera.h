#ifndef CAMERA_H
#define CAMERA_H

#include "core/color.h"
#include "core/rtweekend.h"
#include "core/vec3.h"
#include "external/stb_image_write.h"
#include "geometry/hittable.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <string>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <vector>

class camera {

  public:
	double aspect_ratio = 1.0;	// Ratio of the width over height
	int image_width = 100;		// Rendered image width in pixels
	int samples_per_pixel = 10; // Count of random samples for each pixel
	int max_depth = 10;			// Maximum number of ray bounces into scene
	color background;

	double vfov = 90;				   // Vertical view angle (field of view)
	point3 lookfrom = point3(0, 0, 0); // Point camera is looking from
	point3 lookat = point3(0, 0, -1);  // Point camera is looking at
	vec3 vup = vec3(0, 1, 0);		   // Camera-relative up direction

	double defocus_angle = 0; // Variation angle of rays through each pixel
	double focus_dist = 10;	  // Distance from camera lookfrom to plane of perfect focus

	// Renders the scene to stdout as a PPM image by shooting samples_per_pixel rays per pixel.
	void render(const hittable& world, const std::string& filename = "output.png") {
		initialize();
		std::time_t t = std::time(nullptr);
		char timestamp[16];
		std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&t));

		std::filesystem::create_directories("renders");
		auto dot = filename.find_last_of('.');
		std::string stem = (dot == std::string::npos) ? filename : filename.substr(0, dot);
		std::string ext = (dot == std::string::npos) ? "" : filename.substr(dot);
		std::string out_file
			= "renders/" + stem + "_" + timestamp + "_" + std::to_string(samples_per_pixel) + "spp" + ext;
		// Multithreading the rendering of each row
		std::vector<color> framebuffer(image_width * image_height);

		// std::vector<int> scanlines(image_height);
		// std::iota(scanlines.begin(), scanlines.end(), 0);

		std::atomic<int> pixels_done{0};
		const int total_pixels = image_height * image_width;

		// std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

		tbb::parallel_for(
			tbb::blocked_range2d<int>(0, image_height, 32, 0, image_width, 32),
			[&](const tbb::blocked_range2d<int>& r) {
				for (int j = r.rows().begin(); j < r.rows().end(); j++)
					for (int i = r.cols().begin(); i < r.cols().end(); i++) {
						color pixel_color(0, 0, 0);
						for (int sample = 0; sample < samples_per_pixel; sample++) {
							ray pixel_ray = get_ray(i, j);
							pixel_color += ray_color(pixel_ray, max_depth, world);
						}
						framebuffer[j * image_width + i] = pixel_color * pixel_samples_scale;
					}
				int tile_pixels
					= (r.rows().end() - r.rows().begin()) * (r.cols().end() - r.cols().begin());
				int done = (pixels_done += tile_pixels);
				std::clog << "\rRendering: " << (100 * done / total_pixels) << "%" << std::flush;
			});
		// std::for_each(std::execution::par, scanlines.begin(), scanlines.end(), [&](int j) {
		// 	for (int i = 0; i < image_width; i++) {
		// 		color pixel_color(0, 0, 0);
		// 		for (int sample = 0; sample < samples_per_pixel; sample++) {
		// 			ray r = get_ray(i, j);
		// 			pixel_color += ray_color(r, max_depth, world);
		// 		}
		// 		framebuffer[j * image_width + i] = pixel_color * pixel_samples_scale;
		// 	}
		// 	int done = ++lines_done;
		// 	if (done % 10 == 0)
		// 		std::clog << "\rScanlines done: " << done << "/" << image_height << std::flush;
		// });

		// for (int j = 0; j < image_height; j++) {
		// 	for (int i = 0; i < image_width; i++) {

		// 		write_color(std::cout, framebuffer[j * image_width + i]);
		// 	}
		// }

		std::vector<uint8_t> pixels(image_width * image_height * 3);
		for (int j = 0; j < image_height; j++) {
			for (int i = 0; i < image_width; i++) {
				const color& c = framebuffer[j * image_width + i];
				int idx = (j * image_width + i) * 3;
				static const interval intensity(0.000, 0.999);
				pixels[idx + 0] = uint8_t(256 * intensity.clamp(linear_to_gamma(c.x())));
				pixels[idx + 1] = uint8_t(256 * intensity.clamp(linear_to_gamma(c.y())));
				pixels[idx + 2] = uint8_t(256 * intensity.clamp(linear_to_gamma(c.z())));
			}
		}
		stbi_write_png(out_file.c_str(), image_width, image_height, 3, pixels.data(), image_width * 3);
		std::clog << "\nSaved " << filename << "\n";
	}

  private:
	int image_height;			// Rendered image height
	double pixel_samples_scale; // Color scale factor for a sum of pixel samples
	point3 center;				// Camera center
	point3 pixel00_loc;			// Location of pixel 0,0
	vec3 pixel_delta_u;			// Offset to pixel to the right
	vec3 pixel_delta_v;			// Offset to pixel below
	vec3 u, v, w;				//  Camera frame basis vectors
	vec3 defocus_disk_u;		// Defocus disk horizontal radius
	vec3 defocus_disk_v;		// Defocus disk vertical radius

	// Computes derived camera properties (image height, viewport geometry, pixel grid) from
	// the public configuration fields. Must be called before rendering.
	void initialize() {
		image_height = int(image_width / aspect_ratio);
		image_height = (image_height < 1) ? 1 : image_height;

		pixel_samples_scale = 1.0 / samples_per_pixel;

		center = lookfrom;

		// Determine viewport dimensions
		// auto focal_length = (lookfrom - lookat).length();
		auto theta = degrees_to_radians(vfov);
		auto h = std::tan(theta / 2);
		auto viewport_height = 2 * h * focus_dist;
		auto viewport_width = viewport_height * (double(image_width) / image_height);

		// Calculate the u,v,w unit basis vectors for the camera coord frame.
		w = unit_vector(lookfrom - lookat);
		u = unit_vector(cross(vup, w));
		v = cross(w, u);

		// Calculate the vectors across the horizontal and down the vertical viewport edges.
		auto viewport_u = viewport_width * u;
		auto viewport_v = viewport_height * -v;

		// Calculate the horizontal and vertical delta vectors from pixel to pixel
		pixel_delta_u = viewport_u / image_width;
		pixel_delta_v = viewport_v / image_height;

		// Calculate the location of the upper left pixel
		auto viewport_upper_left = center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;
		pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

		// Calculate the camera defocus disk basis vectors (TODO: learn this)
		auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
		defocus_disk_u = u * defocus_radius;
		defocus_disk_v = v * defocus_radius;
	}

	// Returns a ray from the camera through a random sample point within pixel (i, j).
	ray get_ray(int i, int j) const {
		auto offset = sample_square();
		auto pixel_sample
			= pixel00_loc + ((i + offset.x()) * pixel_delta_u) + ((j + offset.y()) * pixel_delta_v);

		auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
		auto ray_direction = pixel_sample - ray_origin;
		auto ray_time = random_double();

		return ray(ray_origin, ray_direction, ray_time);
	}

	// Returns a random offset vector in the unit square [-0.5, 0.5]^2 for anti-aliasing jitter.
	vec3 sample_square() const {
		return vec3(random_double() - 0.5, random_double() - 0.5, 0);
	}

	// Returns a random point in the camera defocus disk
	point3 defocus_disk_sample() const {
		auto p = random_in_unit_disk();
		return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
	}

	// Recursively traces ray r through the world up to max bounces (depth).
	// Returns the accumulated color contribution. Returns black when depth is exhausted.
	// Falls back to a sky gradient when no object is hit.
	color ray_color(const ray& r, int depth, const hittable& world) const {
		if (depth <= 0) {
			return color(0, 0, 0);
		}
		hit_record rec;

		if (!world.hit(r, interval(0.001, infinity), rec))
			return background;

		ray scattered;
		color attenuation;
		color color_from_emission = rec.mat->emitted(rec.u, rec.v, rec.point);

		if (!rec.mat->scatter(r, rec, attenuation, scattered))
			return color_from_emission;

		color color_from_scatter = attenuation * ray_color(scattered, depth - 1, world);

		return color_from_emission + color_from_scatter;

		// vec3 unit_direction = unit_vector(r.direction());
		// auto a = 0.5 * (unit_direction.y() + 1.0);
		// return (1.0 - a) * color(1.0, 1.0, 1.0) + (a * color(0.5, 0.7, 1.0));
	}
};

#endif
