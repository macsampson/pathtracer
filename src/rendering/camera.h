#ifndef CAMERA_H
#define CAMERA_H

#include "core/color.h"
#include "core/interval.h"
#include "core/rtweekend.h"
#include "core/vec3.h"
#include "external/stb_image_write.h"
#include "geometry/hittable.h"
#include <OpenImageDenoise/oidn.hpp>
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
	bool single_thread = false;

	double vfov = 90;				   // Vertical view angle (field of view)
	point3 lookfrom = point3(0, 0, 0); // Point camera is looking from
	point3 lookat = point3(0, 0, -1);  // Point camera is looking at
	vec3 vup = vec3(0, 1, 0);		   // Camera-relative up direction

	double defocus_angle = 0; // Variation angle of rays through each pixel
	double focus_dist = 10;	  // Distance from camera lookfrom to plane of perfect focus

	double firefly_threshold = 0.0; // 0 = disabled; clamp per-sample luminance above this value
	bool denoise = false;			// run Intel OIDN denoiser after rendering (works on any x86 CPU)

	// Renders the scene to stdout as a PPM image by shooting samples_per_pixel rays per pixel.
	// If exact_path is true, filename is used as-is under renders/ with no timestamp suffix.
	void render(const hittable& world, const hittable& lights,
				const std::string& filename = "output.png", bool exact_path = false) {
		initialize();
		std::filesystem::create_directories("renders");
		std::string out_file;
		if (exact_path) {
			out_file = "renders/" + filename;
		} else {
			std::time_t t = std::time(nullptr);
			char timestamp[16];
			std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&t));
			auto dot = filename.find_last_of('.');
			std::string stem = (dot == std::string::npos) ? filename : filename.substr(0, dot);
			std::string ext = (dot == std::string::npos) ? "" : filename.substr(dot);
			out_file = "renders/" + stem + "_" + timestamp + "_" + std::to_string(samples_per_pixel) + "spp" + ext;
		}
		// Multithreading the rendering of each row
		std::vector<color> framebuffer(image_width * image_height);

		// std::vector<int> scanlines(image_height);
		// std::iota(scanlines.begin(), scanlines.end(), 0);

		std::atomic<int> pixels_done{0};
		const int total_pixels = image_height * image_width;

		// std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

		auto accumulate_pixel = [&](int i, int j, color& pixel_color) {
			for (int sj = 0; sj < sqrt_spp; sj++)
				for (int si = 0; si < sqrt_spp; si++) {
					ray pixel_ray = get_ray(i, j, si, sj);
					color sample_color = ray_color(pixel_ray, max_depth, world, lights, false);
					if (firefly_threshold > 0) {
						double lum = 0.2126 * sample_color.x() + 0.7152 * sample_color.y()
								   + 0.0722 * sample_color.z();
						if (lum > firefly_threshold)
							sample_color *= firefly_threshold / lum;
					}
					pixel_color += sample_color;
				}
		};

		if (single_thread) {
			for (int j = 0; j < image_height; j++)
				for (int i = 0; i < image_width; i++) {
					color pixel_color(0, 0, 0);
					accumulate_pixel(i, j, pixel_color);
					framebuffer[j * image_width + i] = pixel_color * pixel_samples_scale;
					int done = ++pixels_done;
					if (done % (total_pixels / 100 + 1) == 0)
						std::clog << "\rRendering: " << (100 * done / total_pixels) << "%" << std::flush;
				}
		} else {
			tbb::parallel_for(
				tbb::blocked_range2d<int>(0, image_height, 32, 0, image_width, 32),
				[&](const tbb::blocked_range2d<int>& r) {
					for (int j = r.rows().begin(); j < r.rows().end(); j++)
						for (int i = r.cols().begin(); i < r.cols().end(); i++) {
							color pixel_color(0, 0, 0);
							accumulate_pixel(i, j, pixel_color);
							framebuffer[j * image_width + i] = pixel_color * pixel_samples_scale;
						}
					int tile_pixels
						= (r.rows().end() - r.rows().begin()) * (r.cols().end() - r.cols().begin());
					int done = (pixels_done += tile_pixels);
					std::clog << "\rRendering: " << (100 * done / total_pixels) << "%" << std::flush;
				});
		}
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

		if (denoise) {
			std::clog << "\nDenoising..." << std::flush;

			// Build float beauty buffer (OIDN expects linear HDR float32)
			const int npix = image_width * image_height;
			std::vector<float> color_buf(npix * 3);
			for (int k = 0; k < npix; k++) {
				color_buf[k * 3 + 0] = float(framebuffer[k].x());
				color_buf[k * 3 + 1] = float(framebuffer[k].y());
				color_buf[k * 3 + 2] = float(framebuffer[k].z());
			}

			// Render albedo + world-space normal AOVs (single non-jittered primary ray per pixel)
			std::vector<float> albedo_buf(npix * 3, 0.f);
			std::vector<float> normal_buf(npix * 3, 0.f);
			tbb::parallel_for(
				tbb::blocked_range2d<int>(0, image_height, 32, 0, image_width, 32),
				[&](const tbb::blocked_range2d<int>& tile) {
					for (int j = tile.rows().begin(); j < tile.rows().end(); j++) {
						for (int i = tile.cols().begin(); i < tile.cols().end(); i++) {
							int k = j * image_width + i;
							// Centre of pixel, no lens jitter
							auto pixel_sample = pixel00_loc + (double(i) * pixel_delta_u)
											  + (double(j) * pixel_delta_v);
							ray prim(center, pixel_sample - center, 0.0);

							hit_record rec;
							if (world.hit(prim, interval(0.001, infinity), rec)) {
								// Walk through non-diffuse bounces to reach first diffuse surface
								ray cur = prim;
								hit_record cur_rec = rec;
								color throughput(1, 1, 1);
								int aov_depth = 0;
								while (aov_depth++ < 8) {
									ray sc; color att;
									if (!cur_rec.mat->scatter(cur, cur_rec, att, sc))
										break;
									if (cur_rec.mat->is_diffuse() || cur_rec.mat->is_isotropic()) {
										throughput = throughput * att;
										break;
									}
									// Specular bounce — follow it
									throughput = throughput * att;
									hit_record next_rec;
									if (!world.hit(sc, interval(0.001, infinity), next_rec))
										break;
									cur = sc;
									cur_rec = next_rec;
								}
								albedo_buf[k * 3 + 0] = float(throughput.x());
								albedo_buf[k * 3 + 1] = float(throughput.y());
								albedo_buf[k * 3 + 2] = float(throughput.z());
								normal_buf[k * 3 + 0] = float(cur_rec.normal.x());
								normal_buf[k * 3 + 1] = float(cur_rec.normal.y());
								normal_buf[k * 3 + 2] = float(cur_rec.normal.z());
							} else {
								// Background: use sky color as albedo, zero normal
								albedo_buf[k * 3 + 0] = float(background.x());
								albedo_buf[k * 3 + 1] = float(background.y());
								albedo_buf[k * 3 + 2] = float(background.z());
							}
						}
					}
				});

			std::vector<float> out_buf(npix * 3);
			oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CPU);
			device.commit();
			oidn::FilterRef filter = device.newFilter("RT");
			filter.setImage("color",  color_buf.data(),  oidn::Format::Float3, image_width, image_height);
			filter.setImage("albedo", albedo_buf.data(), oidn::Format::Float3, image_width, image_height);
			filter.setImage("normal", normal_buf.data(), oidn::Format::Float3, image_width, image_height);
			filter.setImage("output", out_buf.data(),    oidn::Format::Float3, image_width, image_height);
			filter.set("hdr", true);
			filter.commit();
			filter.execute();

			const char* err;
			if (device.getError(err) != oidn::Error::None)
				std::clog << "\nOIDN error: " << err << "\n";

			// Write denoised floats back into framebuffer
			for (int k = 0; k < npix; k++) {
				framebuffer[k] = color(out_buf[k * 3], out_buf[k * 3 + 1], out_buf[k * 3 + 2]);
			}
		}

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
	int sqrt_spp;				// floor(sqrt(samples_per_pixel)) for stratified grid
	double recip_sqrt_spp;		// 1/sqrt_spp
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

		sqrt_spp = int(std::sqrt(samples_per_pixel));
		recip_sqrt_spp = 1.0 / sqrt_spp;
		pixel_samples_scale = 1.0 / (sqrt_spp * sqrt_spp);

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

	// Returns a ray from the camera through a stratified sample point within pixel (i, j).
	// si, sj index into the sqrt_spp x sqrt_spp stratified grid within the pixel.
	ray get_ray(int i, int j, int si, int sj) const {
		auto offset = sample_square_stratified(si, sj);
		auto pixel_sample
			= pixel00_loc + ((i + offset.x()) * pixel_delta_u) + ((j + offset.y()) * pixel_delta_v);

		auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
		auto ray_direction = pixel_sample - ray_origin;
		auto ray_time = random_double();

		return ray(ray_origin, ray_direction, ray_time);
	}

	// Returns a jittered offset within stratum (si, sj) of a sqrt_spp x sqrt_spp pixel grid.
	// Each stratum covers a 1/sqrt_spp x 1/sqrt_spp region, guaranteeing uniform pixel coverage.
	vec3 sample_square_stratified(int si, int sj) const {
		return vec3(
			(si + random_double()) * recip_sqrt_spp - 0.5,
			(sj + random_double()) * recip_sqrt_spp - 0.5,
			0
		);
	}

	// Returns a random point in the camera defocus disk
	point3 defocus_disk_sample() const {
		auto p = random_in_unit_disk();
		return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
	}

	// Recursively traces ray r through the world up to max bounces (depth).
	// Returns the accumulated color contribution. Returns black when depth is exhausted.
	// Falls back to a sky gradient when no object is hit.
	color ray_color(const ray& r, int depth, const hittable& world, const hittable& lights,
					bool skip_emission) const {
		if (depth <= 0)
			return color(0, 0, 0);

		hit_record rec;
		if (!world.hit(r, interval(0.001, infinity), rec))
			return background;

		color emitted = skip_emission ? color(0, 0, 0) : rec.mat->emitted(rec.u, rec.v, rec.point);

		// If material doesn't scatter (pure emitter), return emission only.
		// But: on a direct camera ray or specular bounce, we DO want to see the light.
		// On a diffuse bounce where we already sampled the light explicitly, we'd
		// double-count. For now, always return emission here — we handle the
		// double-counting below.

		ray scattered;
		color attenuation;
		if (!rec.mat->scatter(r, rec, attenuation, scattered))
			return emitted;

		// === DIRECT LIGHT SAMPLING ===
		// Sample a random point on a light source
		vec3 light_dir = lights.random_point(rec.point) - rec.point;
		// cap min distance
		double distance_sq = light_dir.length_squared();
		distance_sq = std::fmax(distance_sq, 0.01);
		double light_dist = std::sqrt(distance_sq);
		light_dir = unit_vector(light_dir);

		// Check if the light sample is above the surface
		double cos_at_surface = dot(rec.normal, light_dir);
		color direct(0, 0, 0);
		bool is_diffuse = rec.mat->is_diffuse();
		bool is_isotropic = rec.mat->is_isotropic();

		if ((cos_at_surface > 0 && is_diffuse) || is_isotropic) {

			// Shadow ray
			// does anything block the path to the light?
			hit_record blocker_rec;
			ray shadow_ray = ray(rec.point, light_dir, r.time());
			bool blocked = world.hit(shadow_ray, interval(0.001, light_dist - 0.001), blocker_rec);
			if (!blocked) {
				// Light PDF: distance² / (cos_at_light * light_area)
				double light_pdf = lights.pdf_value(rec.point, light_dir);
				if (light_pdf > 0) {
					hit_record light_rec;
					lights.hit(ray(rec.point, light_dir, r.time()), interval(0.001, infinity), light_rec);
					color light_emission
						= light_rec.mat->emitted(light_rec.u, light_rec.v, light_rec.point);
					double weight = is_isotropic ? std::fmin(1.0 / (4.0 * pi * light_pdf), 1.0)
												 : std::fmin(cos_at_surface / (pi * light_pdf), 1.0);
					direct = attenuation * light_emission * weight;
				}
			}
		}

		// === INDIRECT with Russian Roulette ===
		// Terminate low-throughput paths probabilistically (unbiased: boost survivors by 1/prob).
		// Skip RR for the first 2 bounces so primary and secondary rays always run.
		double rr_prob = std::fmax(attenuation.x(), std::fmax(attenuation.y(), attenuation.z()));
		rr_prob = std::fmin(rr_prob, 0.95);
		bool rr_skip = (depth > max_depth - 2);

		color indirect(0, 0, 0);
		if (rr_skip || random_double() < rr_prob) {
			double rr_scale = rr_skip ? 1.0 : 1.0 / rr_prob;
			indirect = (attenuation * rr_scale)
					 * ray_color(scattered, depth - 1, world, lights, is_diffuse || is_isotropic);
		}

		return emitted + direct + indirect;
	}
};

#endif
