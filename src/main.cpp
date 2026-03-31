#include "core/color.h"
#include "geometry/constant_medium.h"
#include "materials/material.h"
#include "geometry/quad.h"
#include "core/rtweekend.h"

#include "geometry/bvh.h"
#include "rendering/camera.h"
#include "geometry/hittable.h"
#include "geometry/hittable_list.h"
#include "geometry/sphere.h"
#include "materials/texture.h"
#include "core/vec3.h"
#include <iostream>
#include <memory>

using std::make_shared;

void scene1(int image_width, int samples_per_pixel, int max_depth);
void scene2(int image_width, int samples_per_pixel, int max_depth);
void moon(int image_width, int samples_per_pixel, int max_depth);
void perlin_spheres(int image_width, int samples_per_pixel, int max_depth);
void quads(int image_width, int samples_per_pixel, int max_depth);
void light_testing(int image_width, int samples_per_pixel, int max_depth);
void cornell_box(int image_width, int samples_per_pixel, int max_depth);
void cornell_box_volumes(int image_width, int samples_per_pixel, int max_depth);
void final_scene(int image_width, int samples_per_pixel, int max_depth);

int main() {
	switch (6) {
	case 1:
		scene1(1200, 10, 20);
		break;
	case 2:
		scene2(400, 100, 50);
		break;
	case 3:
		moon(1200, 100, 50);
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
		cornell_box(400, 100, 50);
		break;
	case 8:
		cornell_box_volumes(800, 200, 50);
		break;
	case 9:
		final_scene(800, 1000, 40);
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
	auto light = make_shared<diffuse_light>(color(15, 15, 15));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
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

void cornell_box_volumes(int image_width, int samples_per_pixel, int max_depth) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(15, 15, 15));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(113, 554, 127), vec3(330, 0, 0), vec3(0, 0, 305), light));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	shared_ptr<hittable> box1 = box(point3(0, 0, 0), point3(165, 165, 165), white);
	box1 = make_shared<rotate_y>(box1, -18);
	box1 = make_shared<translate>(box1, vec3(130, 0, 65));

	shared_ptr<hittable> box2 = box(point3(0, 0, 0), point3(165, 330, 165), white);
	box2 = make_shared<rotate_y>(box2, 15);
	box2 = make_shared<translate>(box2, vec3(295, 0, 295));

	world.add(make_shared<constant_medium>(box1, 0.01, color(1, 1, 1)));
	world.add(make_shared<constant_medium>(box2, 0.01, color(0, 0, 0)));

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

void final_scene(int image_width, int samples_per_pixel, int max_depth) {
	hittable_list boxes1;
	auto ground = make_shared<lambertian>(color(0.48, 0.83, 0.53));

	int boxes_per_side = 20;
	for (int i = 0; i < boxes_per_side; i++) {
		for (int j = 0; j < boxes_per_side; j++) {
			auto w = 100.0;
			auto x0 = -1000.0 + i * w;
			auto z0 = -1000.0 + j * w;
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

	auto center1 = point3(400, 400, 200);
	auto center2 = center1 + vec3(30, 0, 0);
	auto sphere_material = make_shared<lambertian>(color(0.7, 0.3, 0.1));
	world.add(make_shared<sphere>(center1, center2, 50, sphere_material));

	world.add(make_shared<sphere>(point3(260, 150, 45), 50, make_shared<dielectric>(1.5)));
	world.add(
		make_shared<sphere>(point3(0, 150, 145), 50, make_shared<metal>(color(0.8, 0.8, 0.9), 1.0)));

	auto boundary = make_shared<sphere>(point3(360, 150, 145), 70, make_shared<dielectric>(1.5));
	world.add(boundary);
	world.add(make_shared<constant_medium>(boundary, 0.2, color(0.2, 0.4, 0.9)));
	boundary = make_shared<sphere>(point3(0, 0, 0), 5000, make_shared<dielectric>(1.5));
	world.add(make_shared<constant_medium>(boundary, .0001, color(1, 1, 1)));

	auto emat = make_shared<lambertian>(make_shared<image_texture>("assets/moontexture.jpg"));
	world.add(make_shared<sphere>(point3(400, 200, 400), 100, emat));
	auto pertext = make_shared<noise_texture>(0.2);
	world.add(make_shared<sphere>(point3(220, 280, 300), 80, make_shared<lambertian>(pertext)));

	hittable_list boxes2;
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	int ns = 1000;
	for (int j = 0; j < ns; j++) {
		boxes2.add(make_shared<sphere>(point3::random(0, 165), 10, white));
	}

	world.add(make_shared<translate>(make_shared<rotate_y>(make_shared<bvh_node>(boxes2), 15),
									 vec3(-100, 270, 395)));

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
