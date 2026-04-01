#include "core/color.h"
#include "core/rtweekend.h"
#include "geometry/constant_medium.h"
#include "geometry/quad.h"
#include "materials/material.h"

#include "core/vec3.h"
#include "geometry/bvh.h"
#include "geometry/hittable.h"
#include "geometry/hittable_list.h"
#include "geometry/sphere.h"
#include "materials/texture.h"
#include "rendering/camera.h"
#include <memory>
#include <string>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

using std::make_shared;

void space(int image_width, int samples_per_pixel, int max_depth);
void scene2(int image_width, int samples_per_pixel, int max_depth);
void cube_room(int image_width, int samples_per_pixel, int max_depth);
void perlin_spheres(int image_width, int samples_per_pixel, int max_depth);
void quads(int image_width, int samples_per_pixel, int max_depth);
void light_testing(int image_width, int samples_per_pixel, int max_depth);
void cornell_box(int image_width, int samples_per_pixel, int max_depth);
void cornell_box_mirrors(int image_width, int samples_per_pixel, int max_depth);
void cornell_box_2(int image_width, int samples_per_pixel, int max_depth);

int main(int argc, char* argv[]) {
	std::string output_file = (argc > 1) ? argv[1] : "output.png";
	switch (9) {
	case 1:
		space(1600, 10000, 20);
		break;
	case 2:
		scene2(400, 100, 50);
		break;
	case 3:
		cube_room(500, 200, 50);
		break;
	case 4:
		perlin_spheres(800, 1000, 50);
		break;
	case 5:
		quads(400, 100, 50);
		break;
	case 6:
		light_testing(800, 1000, 50);
		break;
	case 7:
		cornell_box(400, 500, 50);
		break;
	case 8:
		cornell_box_mirrors(500, 500, 50);
		break;
	case 9:
		cornell_box_2(500, 500, 40);
		break;
	}
}

void scene1(int image_width, int samples_per_pixel, int max_depth) {
	// World - essentially a list of objects
	hittable_list world;

	// auto material_ground = make_shared<lambertian>(color(0.0, 1.0, 0.5));
	// auto material_center = make_shared<lambertian>(color(0.1, 0.2, 0.5));
	// auto material_left = make_shared<dielectric>(1.50);
	// auto material_bubble = make_shared<dielectric>(1.00 / 1.50);
	// auto material_right = make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);

	// world.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
	// world.add(make_shared<sphere>(point3(0.0, 0.0, -1.2), 0.5, material_center));
	// world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_left));
	// world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.4, material_bubble));
	// world.add(make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_right));

	// auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
	// world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));
	auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));
	world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(checker)));

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			auto choose_mat = random_double();
			point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

			if ((center - point3(4, 0.2, 0)).length() > 0.9) {
				shared_ptr<material> sphere_material;

				if (choose_mat < 0.8) {
					// choose diffuse
					auto albedo = color::random() * color::random();
					sphere_material = make_shared<lambertian>(albedo);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				} else if (choose_mat < 0.95) {
					// choose metal
					auto albedo = color::random(0.5, 1);
					auto fuzz = random_double(0, 0.5);
					sphere_material = make_shared<metal>(albedo, fuzz);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				} else {
					// glass
					sphere_material = make_shared<dielectric>(1.5);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				}
			}
		}
	}

	auto material1 = make_shared<dielectric>(1.5);
	world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

	auto material2 = make_shared<lambertian>(color(0.0, 0.7, 0.5));
	world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

	auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
	world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

	world = hittable_list(make_shared<bvh_node>(world));

	camera cam;

	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = image_width;
	cam.samples_per_pixel = samples_per_pixel;
	cam.max_depth = max_depth;
	cam.background = color(0.7, 0.80, 1.00);

	cam.vfov = 20;
	cam.lookfrom = point3(13, 2, 3);
	cam.lookat = point3(0, 0, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0.6;
	cam.focus_dist = 10.0;

	cam.render(world);
}

void scene2(int image_width, int samples_per_pixel, int max_depth) {
	hittable_list world;

	auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));

	world.add(make_shared<sphere>(point3(0, -10, 0), 10, make_shared<lambertian>(checker)));
	world.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

	camera cam;

	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = image_width;
	cam.samples_per_pixel = samples_per_pixel;
	cam.max_depth = max_depth;
	cam.background = color(0.7, 0.80, 1.00);

	cam.vfov = 20;
	cam.lookfrom = point3(13, 2, 3);
	cam.lookat = point3(0, 0, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;
	// cam.focus_dist = 10.0;

	cam.render(world);
}

void moon(int image_width, int samples_per_pixel, int max_depth) {
	auto moon_texure = make_shared<image_texture>("assets/moontexture.jpg");
	auto moon_surface = make_shared<lambertian>(moon_texure);
	auto globe = make_shared<sphere>(point3(0, 0, 0), 2, moon_surface);

	camera cam;

	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = image_width;
	cam.samples_per_pixel = samples_per_pixel;
	cam.max_depth = max_depth;
	cam.background = color(0.7, 0.80, 1.00);

	cam.vfov = 20;
	cam.lookfrom = point3(0, 0, 12);
	cam.lookat = point3(0, 0, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;

	cam.render(hittable_list(globe));
}

void space(int image_width, int samples_per_pixel, int max_depth) {
	hittable_list space;

	auto moon_texure = make_shared<image_texture>("assets/moon_texture.jpg");
	auto earth_texure = make_shared<image_texture>("assets/earth_texture1.jpg");
	auto mars_texure = make_shared<image_texture>("assets/mars_texture.jpg");

	auto moon_surface = make_shared<lambertian>(moon_texure);
	auto earth_surface = make_shared<lambertian>(earth_texure);
	auto mars_surface = make_shared<lambertian>(mars_texure);

	shared_ptr<hittable> moon = make_shared<sphere>(point3(0, 0, 0), 0.5, moon_surface);
	moon = make_shared<rotate_y>(moon, 0);
	moon = make_shared<translate>(moon, vec3(1, 1, 2));
	space.add(moon);

	shared_ptr<hittable> earth = make_shared<sphere>(point3(0, 0, 0), 2, earth_surface);
	// shared_ptr<hittable> atmosphere
	// 	= make_shared<sphere>(point3(2, 0, 0), 2.1, make_shared<lambertian>(color(0.7, 0.95, 1.0)));
	earth = make_shared<rotate_y>(earth, 180);
	earth = make_shared<translate>(earth, vec3(2, 0, 0));
	// earth = make_shared<constant_medium>(atmosphere, 0.1, color(0.7, 0.95, 1.0));
	space.add(earth);

	shared_ptr<hittable> mars = make_shared<sphere>(point3(0, 0, 0), 0.2, mars_surface);
	mars = make_shared<rotate_y>(mars, 180);
	mars = make_shared<translate>(mars, vec3(-9, 0, -9));
	space.add(mars);

	auto lighting = make_shared<diffuse_light>(color(40, 40, 20));
	space.add(make_shared<sphere>(point3(20, 15, 0), 5, lighting));

	camera cam;

	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = image_width;
	cam.samples_per_pixel = samples_per_pixel;
	cam.max_depth = max_depth;
	cam.background = color(0, 0, 0);

	cam.vfov = 10;
	cam.lookfrom = point3(15, 0, 30);
	cam.lookat = point3(0, 0, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;

	cam.render(space);
}

void perlin_spheres(int image_width, int samples_per_pixel, int max_depth) {
	hittable_list world;

	auto pertext = make_shared<noise_texture>(4);

	world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(pertext)));
	world.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<lambertian>(pertext)));

	camera cam;

	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = image_width;
	cam.samples_per_pixel = samples_per_pixel;
	cam.max_depth = max_depth;
	cam.background = color(0.7, 0.80, 1.00);

	cam.vfov = 20;
	cam.lookfrom = point3(13, 2, 3);
	cam.lookat = point3(0, 0, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;

	cam.render(world);
}

void quads(int image_width, int samples_per_pixel, int max_depth) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(1.0, 0.2, 0.2));
	auto green = make_shared<lambertian>(color(0.2, 1.0, 0.2));
	auto blue = make_shared<lambertian>(color(0.2, 0.2, 1.0));
	auto orange = make_shared<lambertian>(color(1.0, 0.5, 0.0));
	auto teal = make_shared<lambertian>(color(0.2, 0.8, 0.8));

	world.add(make_shared<quad>(point3(-3, -2, 5), vec3(0, 0, -4), vec3(0, 4, 0), red));
	world.add(make_shared<quad>(point3(-2, -2, 0), vec3(4, 0, 0), vec3(0, 4, 0), green));
	world.add(make_shared<quad>(point3(3, -2, 1), vec3(0, 0, 4), vec3(0, 4, 0), blue));
	world.add(make_shared<quad>(point3(-2, 3, 1), vec3(4, 0, 0), vec3(0, 0, 4), orange));
	world.add(make_shared<quad>(point3(-2, -3, 5), vec3(4, 0, 0), vec3(0, 0, -4), teal));

	camera cam;

	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = image_width;
	cam.samples_per_pixel = samples_per_pixel;
	cam.max_depth = max_depth;
	cam.background = color(0.7, 0.80, 1.00);

	cam.vfov = 80;
	cam.lookfrom = point3(0, 0, 9);
	cam.lookat = point3(0, 0, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;

	cam.render(world);
}

void light_testing(int image_width, int samples_per_pixel, int max_depth) {
	hittable_list world;

	auto pertext = make_shared<noise_texture>(4);
	auto moon_texure = make_shared<image_texture>("assets/moontexture.jpg");

	world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(pertext)));
	world.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<lambertian>(pertext)));

	auto lighting = make_shared<diffuse_light>(color(20, 20, 20));
	world.add(make_shared<quad>(point3(3, 1, -2), vec3(2, 0, 0), vec3(0, 2, 0), lighting));
	// world.add(make_shared<sphere>(point3(7, 7, 0), 1, lighting));

	camera cam;

	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = image_width;
	cam.samples_per_pixel = samples_per_pixel;
	cam.max_depth = max_depth;
	cam.background = color(0, 0, 0);

	cam.vfov = 20;
	cam.lookfrom = point3(26, 3, 6);
	cam.lookat = point3(0, 2, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;

	cam.render(world);
}

void cornell_box(int image_width, int samples_per_pixel, int max_depth) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(30, 30, 30));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	world.add(make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), light));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	shared_ptr<hittable> box1 = box(point3(0, 0, 0), point3(165, 165, 165), white);
	box1 = make_shared<rotate_y>(box1, -18);
	box1 = make_shared<translate>(box1, vec3(130, 0, 65));
	world.add(box1);

	shared_ptr<hittable> box2 = box(point3(0, 0, 0), point3(165, 330, 165), white);
	box2 = make_shared<rotate_y>(box2, 15);
	box2 = make_shared<translate>(box2, vec3(295, 0, 295));
	world.add(box2);

	camera cam;

	cam.aspect_ratio = 1.0;
	cam.image_width = image_width;
	cam.samples_per_pixel = samples_per_pixel;
	cam.max_depth = max_depth;
	cam.background = color(0, 0, 0);

	cam.vfov = 40;
	cam.lookfrom = point3(278, 278, -800);
	cam.lookat = point3(278, 278, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;

	cam.render(world);
}

void cornell_box_2(int image_width, int samples_per_pixel, int max_depth) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(30, 30, 30));
	auto red_metal = make_shared<metal>(color(.65, .05, .05), 0.5);
	auto green_metal = make_shared<metal>(color(.12, .45, .15), 0.5);

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red_metal));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green_metal));
	world.add(make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), light));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	shared_ptr<hittable> glass_sphere
		= make_shared<sphere>(point3(0, 0, 0), 75, make_shared<dielectric>(1.5));
	// box1 = make_shared<rotate_y>(box1, -18);
	glass_sphere = make_shared<translate>(glass_sphere, vec3(150, 75, 200));
	world.add(glass_sphere);

	shared_ptr<hittable> mirror_sphere
		= make_shared<sphere>(point3(0, 0, 0), 150, make_shared<metal>(color(0.5, 0.5, 0.5), 0));
	// box1 = make_shared<rotate_y>(box1, -18);
	mirror_sphere = make_shared<translate>(mirror_sphere, vec3(0, 555, 555));
	world.add(mirror_sphere);

	// shared_ptr<hittable> cloud_sphere = make_shared<sphere>(point3(0, 0, 0), 100, white);
	// // box1 = make_shared<rotate_y>(box1, -18);
	// cloud_sphere = make_shared<translate>(cloud_sphere, vec3(450, 450, 400));
	// world.add(make_shared<constant_medium>(cloud_sphere, 0.01, color(.12, 0, .15)));

	shared_ptr<hittable> tall_yellow_box
		= box(point3(0, 0, 0), point3(70, 300, 70), make_shared<metal>(color(0.5, 0.5, 0.0), 0.5));
	tall_yellow_box = make_shared<rotate_y>(tall_yellow_box, -45);
	tall_yellow_box = make_shared<translate>(tall_yellow_box, vec3(50, 0, 450));
	world.add(make_shared<constant_medium>(tall_yellow_box, 0.05, color(1.0, 0.75, 0.2)));

	shared_ptr<hittable> large_mirror_box
		= box(point3(0, 0, 0), point3(200, 200, 200), make_shared<metal>(color(1.0, 1.0, 1.0), 0.1));
	large_mirror_box = make_shared<rotate_y>(large_mirror_box, 45);
	large_mirror_box = make_shared<translate>(large_mirror_box, vec3(250, 0, 300));
	world.add(large_mirror_box);

	shared_ptr<hittable> small_lambertian_box
		= box(point3(0, 0, 0), point3(100, 200, 100), make_shared<diffuse_light>(color(0.7, 0.95, 1.0)));
	small_lambertian_box = make_shared<rotate_y>(small_lambertian_box, -20);
	small_lambertian_box = make_shared<translate>(small_lambertian_box, vec3(350, 200, 250));
	world.add(make_shared<constant_medium>(small_lambertian_box, 0.3, color(0.7, 0.95, 1.0)));

	camera cam;

	cam.aspect_ratio = 1.0;
	cam.image_width = image_width;
	cam.samples_per_pixel = samples_per_pixel;
	cam.max_depth = max_depth;
	cam.background = color(0, 0, 0);

	cam.vfov = 40;
	cam.lookfrom = point3(278, 278, -800);
	cam.lookat = point3(278, 278, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;

	cam.render(world);
}

void cornell_box_mirrors(int image_width, int samples_per_pixel, int max_depth) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(3, 5, 20));
	auto red_metal = make_shared<metal>(color(.65, .05, .05), 0);
	auto green_metal = make_shared<metal>(color(.12, .45, .15), 0);

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red_metal));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green_metal));
	world.add(make_shared<quad>(point3(113, 554, 127), vec3(330, 0, 0), vec3(0, 0, 305), light));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	const double radius = 75;
	std::vector<point3> placed;

	for (int a = 0; a < 5; a++) {
		for (int b = 0; b < 5; b++) {
			auto choose_mat = random_double();
			point3 center(radius + (a * 100) + random_double(0, 30), random_double(radius, 555 - radius),
						  radius + (b * 100) + random_double(0, 30));

			bool overlaps = false;

			for (const auto& other : placed) {
				if ((center - other).length() < 2 * radius) {
					overlaps = true;
					break;
				}
			}

			if (overlaps)
				continue;
			placed.push_back(center);

			shared_ptr<material> sphere_material;
			color volume_albedo;
			double volume_density = 0;
			bool volume = false;

			// if (choose_mat < 0.0) {
			// 	// choose diffuse
			// 	auto albedo = color::random() * color::random();
			// 	sphere_material = make_shared<lambertian>(albedo);
			// } else
			if (choose_mat < 0.4) {
				// choose metal
				auto albedo = color::random(0.05, 1);
				auto fuzz = random_double(0, 0.1);
				sphere_material = make_shared<metal>(albedo, fuzz);
			} else if (choose_mat < 0.52) {
				// emissive
				auto albedo = color::random(0.1, 1.5);
				sphere_material = make_shared<diffuse_light>(albedo);
			} else if (choose_mat < 0.8) {
				// volume
				volume_albedo = color::random(0, 1);
				volume_density = random_double(0.01, 0.1);
				volume = true;
			} else {
				// glass
				sphere_material = make_shared<dielectric>(1.5);
			}

			shared_ptr<hittable> s = make_shared<sphere>(center, radius, sphere_material);
			if (volume)
				world.add(make_shared<constant_medium>(s, volume_density, volume_albedo));
			else
				world.add(s);
		}
	}

	// shared_ptr<hittable> glass_sphere
	// 	= make_shared<sphere>(point3(0, 0, 0), 75, make_shared<dielectric>(1.5));
	// // box1 = make_shared<rotate_y>(box1, -18);
	// glass_sphere = make_shared<translate>(glass_sphere, vec3(450, 225, 30));
	// world.add(glass_sphere);

	// shared_ptr<hittable> lambertian_sphere
	// 	= make_shared<sphere>(point3(0, 0, 0), 75, make_shared<lambertian>(color(0.7, 0.95, 1.0)));
	// // box1 = make_shared<rotate_y>(box1, -18);
	// lambertian_sphere = make_shared<translate>(lambertian_sphere, vec3(400, 225, 130));
	// world.add(lambertian_sphere);

	// shared_ptr<hittable> metal_sphere
	// 	= make_shared<sphere>(point3(0, 0, 0), 75, make_shared<metal>(color(0.5, 0.5, 0.5), 0));
	// // box1 = make_shared<rotate_y>(box1, -18);
	// metal_sphere = make_shared<translate>(metal_sphere, vec3(350, 225, 205));
	// world.add(metal_sphere);

	// shared_ptr<hittable> large_glass_sphere
	// 	= make_shared<sphere>(point3(0, 0, 0), 150, make_shared<dielectric>(1.5));
	// // box1 = make_shared<rotate_y>(box1, -18);
	// large_glass_sphere = make_shared<translate>(large_glass_sphere, vec3(250, 150, 300));
	// world.add(large_glass_sphere);

	camera cam;

	cam.aspect_ratio = 1.0;
	cam.image_width = image_width;
	cam.samples_per_pixel = samples_per_pixel;
	cam.max_depth = max_depth;
	cam.background = color(0, 0, 0);

	cam.vfov = 40;
	cam.lookfrom = point3(278, 278, -800);
	cam.lookat = point3(278, 278, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;

	cam.render(world);
}

void cube_room(int image_width, int samples_per_pixel, int max_depth) {
	hittable_list boxes1;

	int boxes_per_side = 20;
	for (int i = 0; i < boxes_per_side; i++) {
		for (int j = 0; j < boxes_per_side; j++) {
			auto ground = make_shared<lambertian>(color::random());
			auto w = 100.0;
			auto x0 = -500.0 + i * w;
			auto z0 = -500.0 + j * w;
			auto y0 = 0.0;
			auto x1 = x0 + w;
			auto y1 = random_double(1, 101);
			auto z1 = z0 + w;

			boxes1.add(box(point3(x0, y0, z0), point3(x1, y1, z1), ground));
		}
	}

	hittable_list world;

	world.add(make_shared<bvh_node>(boxes1));

	auto light = make_shared<diffuse_light>(color(7, 7, 7));
	world.add(make_shared<quad>(point3(123, 554, 147), vec3(300, 0, 0), vec3(0, 0, 265), light));

	// auto center1 = point3(400, 400, 200);
	// auto center2 = center1 + vec3(30, 0, 0);
	// auto sphere_material = make_shared<lambertian>(color(0.7, 0.3, 0.1));
	// world.add(make_shared<sphere>(center1, center2, 50, sphere_material));

	// world.add(make_shared<sphere>(point3(260, 150, 45), 50, make_shared<dielectric>(1.5)));
	// world.add(
	// 	make_shared<sphere>(point3(0, 150, 145), 50, make_shared<metal>(color(0.8, 0.8, 0.9), 1.0)));

	// auto boundary = make_shared<sphere>(point3(360, 150, 145), 70, make_shared<dielectric>(1.5));
	// world.add(boundary);
	// world.add(make_shared<constant_medium>(boundary, 0.2, color(0.2, 0.4, 0.9)));
	// boundary = make_shared<sphere>(point3(0, 0, 0), 5000, make_shared<dielectric>(1.5));
	// world.add(make_shared<constant_medium>(boundary, .0001, color(1, 1, 1)));

	// auto emat = make_shared<lambertian>(make_shared<image_texture>("assets/moontexture.jpg"));
	// world.add(make_shared<sphere>(point3(400, 200, 400), 100, emat));
	// auto pertext = make_shared<noise_texture>(0.2);
	// world.add(make_shared<sphere>(point3(220, 280, 300), 80, make_shared<lambertian>(pertext)));

	// hittable_list boxes2;
	// auto white = make_shared<lambertian>(color(.73, .73, .73));
	// int ns = 1000;
	// for (int j = 0; j < ns; j++) {
	// 	boxes2.add(make_shared<sphere>(point3::random(0, 165), 10, white));
	// }

	// world.add(make_shared<translate>(make_shared<rotate_y>(make_shared<bvh_node>(boxes2), 15),
	// 								 vec3(-100, 270, 395)));

	camera cam;

	cam.aspect_ratio = 1.0;
	cam.image_width = image_width;
	cam.samples_per_pixel = samples_per_pixel;
	cam.max_depth = max_depth;
	cam.background = color(0, 0, 0);

	cam.vfov = 40;
	cam.lookfrom = point3(478, 278, -600);
	cam.lookat = point3(278, 278, 0);
	cam.vup = vec3(0, 1, 0);

	cam.defocus_angle = 0;

	cam.render(world);
}
