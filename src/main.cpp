#include "core/color.h"
#include "core/rtweekend.h"
#include "geometry/constant_medium.h"
#include "geometry/mesh.h"
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

void space(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void scene2(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void cube_room(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void perlin_spheres(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void glass_cube(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void light_testing(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void cornell_box(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void cornell_random_spheres(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void cornell_variety(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void majora_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void midna_tp_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void midna_hw_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void dragon_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void armadillo_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void suzanne_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void suzanne_armadillo_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void adult_link_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
void suzanne_glass_spin(int image_width, int samples_per_pixel, int max_depth, bool single_thread);
// TODO: make a scene with a glass cube full of metal spheres.

struct Scene {
	const char* name;
	void (*fn)(int, int, int, bool);
};

int main(int argc, char* argv[]) {
	Scene scenes[] = {
		{"space", space},
		{"scene2", scene2},
		{"cube_room", cube_room},
		{"perlin_spheres", perlin_spheres},
		{"glass_cube", glass_cube},
		{"light_testing", light_testing},
		{"cornell_box", cornell_box},
		{"cornell_random", cornell_random_spheres},
		{"cornell_variety", cornell_variety},
		{"majora_mesh", majora_mesh},
		{"midna_tp_mesh", midna_tp_mesh},
		{"midna_hw_mesh", midna_hw_mesh},
		{"dragon_mesh", dragon_mesh},
		{"armadillo_mesh", armadillo_mesh},
		{"suzanne_mesh", suzanne_mesh},
		{"adult_link_mesh", adult_link_mesh},
		{"suzanne_armadillo_mesh", suzanne_armadillo_mesh},
		{"suzanne_glass_spin", suzanne_glass_spin},
	};

	// Defaults
	std::string scene_name = "cornell_random";
	int image_width = 100;
	int samples_per_pixel = 100;
	int max_depth = 30;
	bool single_thread = false;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if ((arg == "-s" || arg == "--scene") && i + 1 < argc)
			scene_name = argv[++i];
		else if ((arg == "-w" || arg == "--width") && i + 1 < argc)
			image_width = std::stoi(argv[++i]);
		else if (arg == "--spp" && i + 1 < argc)
			samples_per_pixel = std::stoi(argv[++i]);
		else if ((arg == "-d" || arg == "--depth") && i + 1 < argc)
			max_depth = std::stoi(argv[++i]);
		else if (arg == "--single-thread")
			single_thread = true;
		else if (arg == "--list") {
			for (const auto& s : scenes)
				std::cout << "  " << s.name << "\n";
			return 0;
		} else {
			std::cerr << "Usage: raytracing [-s scene] [-w width] [--spp N] [-d depth] [--single-thread] "
						 "[--list]\n";
			return 1;
		}
	}

	for (const auto& s : scenes) {
		if (scene_name == s.name) {
			s.fn(image_width, samples_per_pixel, max_depth, single_thread);
			return 0;
		}
	}
	std::cerr << "Unknown scene: " << scene_name << "\nUse --list to see available scenes.\n";
	return 1;
}

void scene1(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
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

	hittable_list lights;
	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void scene2(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));

	world.add(make_shared<sphere>(point3(0, -10, 0), 10, make_shared<lambertian>(checker)));
	world.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

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

	cam.defocus_angle = 0;
	// cam.focus_dist = 10.0;

	hittable_list lights;
	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void moon(int image_width, int samples_per_pixel, int max_depth) {
	auto moon_texure = make_shared<image_texture>("assets/moon_texture.jpg");
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

	hittable_list lights;
	cam.render(hittable_list(globe), lights);
}

void space(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
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
	auto light_sphere = make_shared<sphere>(point3(20, 15, 0), 5, lighting);
	space.add(light_sphere);

	hittable_list lights;
	lights.add(light_sphere);

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

	cam.single_thread = single_thread;
	cam.render(space, lights);
}

void perlin_spheres(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
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

	hittable_list lights;
	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void glass_cube(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(7, 7, 7));
	auto material_glass = make_shared<dielectric>(1.50);
	auto material_bubble = make_shared<dielectric>(1.00 / 1.50);
	auto metal_mat = make_shared<metal>(color(0.8, 0.8, 0.9), 0.0);
	auto red_metal = make_shared<metal>(color(.65, .05, .05), 0.5);
	auto green_metal = make_shared<metal>(color(.12, .45, .15), 0.5);
	auto blue_metal = make_shared<metal>(color(.12, .25, .75), 0.0);

	double cube_size = 230;
	double inner_cube_size = 0.9 * cube_size;
	double offset = (cube_size - inner_cube_size) / 2.0;

	// Shared rotation and translation for both cubes and spheres
	double rot_angle = 20;
	vec3 translate_pos(135, 165, 165);

	// Cornell box walls
	// world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), white));
	// world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), blue_metal));
	auto light_quad = make_shared<quad>(point3(113, 554, 127), vec3(330, 0, 0), vec3(0, 0, 305), light);
	world.add(light_quad);
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(1000, 0, 0), vec3(0, 555, 0), white));

	hittable_list lights;
	lights.add(light_quad);

	// Outer glass cube
	shared_ptr<hittable> box1
		= box(point3(0, 0, 0), point3(cube_size, cube_size, cube_size), material_glass);
	box1 = make_shared<rotate_y>(box1, rot_angle);
	box1 = make_shared<translate>(box1, translate_pos);
	world.add(box1);

	// Inner bubble cube (centered inside outer)
	shared_ptr<hittable> box2
		= box(point3(offset, offset, offset),
			  point3(offset + inner_cube_size, offset + inner_cube_size, offset + inner_cube_size),
			  material_bubble);
	box2 = make_shared<rotate_y>(box2, rot_angle);
	box2 = make_shared<translate>(box2, translate_pos);
	world.add(box2);

	// FCC close-packed metal spheres inside inner cube
	double sphere_radius = 20;
	double a = 2.0 * sqrt(2.0) * sphere_radius; // FCC lattice parameter

	// FCC basis: 4 atoms per unit cell (fractional coordinates)
	double basis[4][3] = {
		{0.0, 0.0, 0.0},
		{0.5, 0.5, 0.0},
		{0.5, 0.0, 0.5},
		{0.0, 0.5, 0.5},
	};

	double inner_min = offset + sphere_radius;
	double inner_max = offset + inner_cube_size - sphere_radius;
	int cells = static_cast<int>(ceil(inner_cube_size / a)) + 1;

	for (int i = 0; i < cells; i++) {
		for (int j = 0; j < cells; j++) {
			for (int k = 0; k < cells; k++) {
				for (int b = 0; b < 4; b++) {
					double x = offset + (i + basis[b][0]) * a;
					double y = offset + (j + basis[b][1]) * a;
					double z = offset + (k + basis[b][2]) * a;

					if (x >= inner_min && x <= inner_max && y >= inner_min && y <= inner_max
						&& z >= inner_min && z <= inner_max) {
						shared_ptr<hittable> s = make_shared<sphere>(
							point3(x, y, z), sphere_radius, make_shared<metal>(color::random(), 0.0));
						s = make_shared<rotate_y>(s, rot_angle);
						s = make_shared<translate>(s, translate_pos);
						world.add(s);
					}
				}
			}
		}
	}

	world = hittable_list(make_shared<bvh_node>(world));

	camera cam;

	cam.aspect_ratio = 1.0;
	cam.image_width = image_width;
	cam.samples_per_pixel = samples_per_pixel;
	cam.max_depth = max_depth;
	cam.background = color(0, 0, 0);

	cam.vfov = 40;
	cam.lookfrom = point3(900, 278, -500);
	cam.lookat = point3(500, 278, 0);
	cam.vup = vec3(0, 1, 0);

	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void light_testing(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto pertext = make_shared<noise_texture>(4);
	auto moon_texure = make_shared<image_texture>("assets/moon_texture.jpg");

	world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(pertext)));
	world.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<lambertian>(pertext)));

	auto lighting = make_shared<diffuse_light>(color(20, 20, 20));
	auto light_quad = make_shared<quad>(point3(3, 1, -2), vec3(2, 0, 0), vec3(0, 2, 0), lighting);
	world.add(light_quad);
	// world.add(make_shared<sphere>(point3(7, 7, 0), 1, lighting));

	hittable_list lights;
	lights.add(light_quad);

	world = hittable_list(make_shared<bvh_node>(world));

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

	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void cornell_box(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(30, 30, 30));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	auto light_quad = make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), light);
	world.add(light_quad);
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	hittable_list lights;
	lights.add(light_quad);

	shared_ptr<hittable> box1 = box(point3(0, 0, 0), point3(165, 165, 165), white);
	box1 = make_shared<rotate_y>(box1, -18);
	box1 = make_shared<translate>(box1, vec3(130, 0, 65));
	world.add(box1);

	shared_ptr<hittable> box2 = box(point3(0, 0, 0), point3(165, 330, 165), white);
	box2 = make_shared<rotate_y>(box2, 15);
	box2 = make_shared<translate>(box2, vec3(295, 0, 295));
	world.add(box2);

	world = hittable_list(make_shared<bvh_node>(world));

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

	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void cornell_variety(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(7, 7, 7));
	auto red_metal = make_shared<metal>(color(.65, .05, .05), 0.5);
	auto green_metal = make_shared<metal>(color(.12, .45, .15), 0.5);
	auto brushed_metal = make_shared<metal>(color(1.0, 1.0, 1.0), 0.05);

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	auto light_quad = make_shared<quad>(point3(113, 554, 127), vec3(330, 0, 0), vec3(0, 0, 305), light);
	world.add(light_quad);
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));
	// world.add(make_shared<quad>(point3(0, 0, ), vec3(555, 0, 0), vec3(0, 555, 0), white));

	hittable_list lights;
	lights.add(light_quad);

	shared_ptr<hittable> glass_sphere
		= make_shared<sphere>(point3(0, 0, 0), 70, make_shared<dielectric>(1.5));
	// box1 = make_shared<rotate_y>(box1, -18);
	glass_sphere = make_shared<translate>(glass_sphere, vec3(170, 75, 150));
	world.add(glass_sphere);

	shared_ptr<hittable> mirror_sphere
		= make_shared<sphere>(point3(0, 0, 0), 200, make_shared<metal>(color(0.5, 0.5, 0.5), 0));
	// box1 = make_shared<rotate_y>(box1, -18);
	mirror_sphere = make_shared<translate>(mirror_sphere, vec3(0, 277, 555));
	world.add(mirror_sphere);

	shared_ptr<hittable> secret_sphere
		= make_shared<sphere>(point3(0, 0, 0), 60, make_shared<metal>(color(0.5, 0.1, 0.5), 0));
	// box1 = make_shared<rotate_y>(box1, -18);
	secret_sphere = make_shared<translate>(secret_sphere, vec3(320, 60, 470));
	world.add(secret_sphere);

	// shared_ptr<hittable> cloud_sphere = make_shared<sphere>(point3(0, 0, 0), 100, white);
	// // box1 = make_shared<rotate_y>(box1, -18);
	// cloud_sphere = make_shared<translate>(cloud_sphere, vec3(450, 450, 400));
	// world.add(make_shared<constant_medium>(cloud_sphere, 0.01, color(.12, 0, .15)));

	shared_ptr<hittable> tall_yellow_box
		= box(point3(0, 0, 0), point3(70, 300, 70), make_shared<metal>(color(0.5, 0.5, 0.0), 0.5));
	tall_yellow_box = make_shared<rotate_y>(tall_yellow_box, -45);
	tall_yellow_box = make_shared<translate>(tall_yellow_box, vec3(500, 0, 100));
	world.add(make_shared<constant_medium>(tall_yellow_box, 0.02, color(1.0, 0.75, 0.2)));

	shared_ptr<hittable> large_mirror_box
		= box(point3(0, 0, 0), point3(200, 200, 200), make_shared<lambertian>(color(0.1, 0.3, 1.0)));
	large_mirror_box = make_shared<rotate_y>(large_mirror_box, 45);
	large_mirror_box = make_shared<translate>(large_mirror_box, vec3(250, 0, 300));
	world.add(large_mirror_box);

	shared_ptr<hittable> small_lambertian_box
		= box(point3(0, 0, 0), point3(100, 200, 100), make_shared<diffuse_light>(color(0.7, 0.95, 1.0)));
	small_lambertian_box = make_shared<rotate_y>(small_lambertian_box, -20);
	small_lambertian_box = make_shared<translate>(small_lambertian_box, vec3(350, 200, 250));
	world.add(make_shared<constant_medium>(small_lambertian_box, 0.3, color(0.7, 0.95, 1.0)));

	world = hittable_list(make_shared<bvh_node>(world));

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

	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void cornell_random_spheres(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(2, 5, 20));
	auto red_metal = make_shared<metal>(color(.65, .05, .05), 0);
	auto green_metal = make_shared<metal>(color(.12, .45, .15), 0);
	auto white_metal = make_shared<metal>(color(0.5, 0.5, 0.5), 0.1);

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red_metal));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green_metal));
	auto light_quad = make_shared<quad>(point3(113, 554, 127), vec3(330, 0, 0), vec3(0, 0, 305), light);
	world.add(light_quad);
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	hittable_list lights;
	lights.add(light_quad);

	const double radius = 75;
	std::vector<point3> placed;

	while (placed.size() < 16) {
		for (int a = 0; a < 7; a++) {
			for (int b = 0; b < 7; b++) {
				auto choose_mat = random_double();
				double cx = std::clamp(radius + (a * 100) + random_double(0, 30), radius, 555.0 - radius);
				double cz = std::clamp(radius + (b * 100) + random_double(0, 30), radius, 555.0 - radius);
				point3 center(cx, random_double(radius, 555 - radius), cz);

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
				} else if (choose_mat < 0.5) {
					// emissive
					auto albedo = color::random(0.7, 5.0);
					// auto albedo = color(0.2, 2.0, 3.0);
					sphere_material = make_shared<diffuse_light>(albedo);
				} else if (choose_mat < 0.6) {
					// volume
					volume_albedo = color::random(0, 1);
					volume_density = random_double(0.1, 0.15);
					volume = true;
				} else {
					// glass
					sphere_material = make_shared<dielectric>(1.5);
				}

				shared_ptr<hittable> s = make_shared<sphere>(center, radius, sphere_material);
				if (volume)
					world.add(make_shared<constant_medium>(s, volume_density, volume_albedo));
				else {
					world.add(s);
					if (choose_mat >= 0.4 && choose_mat < 0.5) // emissive
						lights.add(s);
				}
			}
		}
	}

	world = hittable_list(make_shared<bvh_node>(world));

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

	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void cube_room(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
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
	auto light_quad = make_shared<quad>(point3(123, 554, 147), vec3(300, 0, 0), vec3(0, 0, 265), light);
	world.add(light_quad);

	hittable_list lights;
	lights.add(light_quad);

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

	world = hittable_list(make_shared<bvh_node>(world));
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

	cam.single_thread = single_thread;
	cam.render(world, lights);
}

// Renders an OBJ file inside a Cornell box. Place your OBJ at assets/model.obj.
// load_obj_fit auto-scales to fit; adjust the translate to reposition as needed.
void majora_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(15, 15, 15));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	auto light_quad = make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), light);
	world.add(light_quad);
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	shared_ptr<hittable> light_sphere
		= make_shared<sphere>(point3(0, 0, 0), 70, make_shared<diffuse_light>(color(15, 15, 15)));
	light_sphere = make_shared<translate>(light_sphere, vec3(470, 75, 150));
	world.add(light_sphere);

	hittable_list lights;
	lights.add(light_quad);
	lights.add(light_sphere);

	auto mesh_mat = make_shared<lambertian>(color(0.8, 0.7, 0.6));
	auto glass_mat = make_shared<dielectric>(1.5);
	auto red_metal = make_shared<metal>(color(.65, .05, .05), 0);
	auto obj = load_obj_fit("assets/majora/MajorasMask.obj", mesh_mat, 500.0, true, glass_mat);
	// obj = make_shared<rotate_x>(obj, -90);
	obj = make_shared<rotate_y>(obj, 180);
	// obj = make_shared<rotate_z>(obj, 45);
	obj = make_shared<translate>(obj, vec3(278, 0, 278));
	world.add(obj);

	world = hittable_list(make_shared<bvh_node>(world));

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
	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void midna_tp_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(15, 15, 15));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	auto light_quad = make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), light);
	world.add(light_quad);
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	shared_ptr<hittable> light_sphere
		= make_shared<sphere>(point3(0, 0, 0), 70, make_shared<diffuse_light>(color(15, 15, 15)));
	light_sphere = make_shared<translate>(light_sphere, vec3(470, 75, 150));
	world.add(light_sphere);

	hittable_list lights;
	lights.add(light_quad);
	lights.add(light_sphere);

	auto mesh_mat = make_shared<lambertian>(color(0.8, 0.7, 0.6));
	auto glass_mat = make_shared<dielectric>(1.5);
	auto red_metal = make_shared<metal>(color(.65, .05, .05), 0);
	auto obj = load_obj_fit("assets/midna_tp/midna.obj", mesh_mat, 700.0, true, nullptr, true, false);
	// obj = make_shared<rotate_x>(obj, -90);
	obj = make_shared<rotate_y>(obj, 180);
	// obj = make_shared<rotate_z>(obj, 45);
	obj = make_shared<translate>(obj, vec3(278, 0, 278));
	world.add(obj);

	world = hittable_list(make_shared<bvh_node>(world));

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
	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void midna_hw_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(15, 15, 15));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	auto light_quad = make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), light);
	world.add(light_quad);
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	shared_ptr<hittable> light_sphere
		= make_shared<sphere>(point3(0, 0, 0), 70, make_shared<diffuse_light>(color(15, 15, 15)));
	light_sphere = make_shared<translate>(light_sphere, vec3(470, 75, 150));
	world.add(light_sphere);

	hittable_list lights;
	lights.add(light_quad);
	lights.add(light_sphere);

	auto mesh_mat = make_shared<lambertian>(color(0.8, 0.7, 0.6));
	auto glass_mat = make_shared<dielectric>(1.5);
	auto red_metal = make_shared<metal>(color(.65, .05, .05), 0);
	auto obj = load_obj_fit("assets/midna_hw/C_MIDNA_PRINCESS.obj", mesh_mat, 500.0, true, nullptr, true,
							false);
	// obj = make_shared<rotate_x>(obj, -90);
	obj = make_shared<rotate_y>(obj, 180);
	// obj = make_shared<rotate_z>(obj, 45);
	obj = make_shared<translate>(obj, vec3(278, 215, 278));
	world.add(obj);

	world = hittable_list(make_shared<bvh_node>(world));

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
	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void dragon_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(15, 15, 15));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	auto light_quad = make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), light);
	world.add(light_quad);
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	shared_ptr<hittable> light_sphere
		= make_shared<sphere>(point3(0, 0, 0), 35, make_shared<diffuse_light>(color(15, 15, 15)));
	light_sphere = make_shared<translate>(light_sphere, vec3(470, 75, 150));
	world.add(light_sphere);

	hittable_list lights;
	lights.add(light_quad);
	lights.add(light_sphere);

	auto mesh_mat = make_shared<lambertian>(color(0.8, 0.7, 0.6));
	auto glass_mat = make_shared<dielectric>(1.5);
	auto red_metal = make_shared<metal>(color(.65, .05, .05), 0);
	auto obj = load_obj_fit("assets/dragon.obj", mesh_mat, 500.0, true, glass_mat);
	obj = make_shared<rotate_x>(obj, -90);
	obj = make_shared<rotate_y>(obj, 200);
	// obj = make_shared<rotate_z>(obj, 45);
	obj = make_shared<translate>(obj, vec3(278, 278, 278));
	world.add(obj);

	world = hittable_list(make_shared<bvh_node>(world));

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
	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void armadillo_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(15, 15, 15));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	auto light_quad = make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), light);
	world.add(light_quad);
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	shared_ptr<hittable> light_sphere
		= make_shared<sphere>(point3(0, 0, 0), 35, make_shared<diffuse_light>(color(15, 15, 15)));
	light_sphere = make_shared<translate>(light_sphere, vec3(470, 75, 150));
	world.add(light_sphere);

	hittable_list lights;
	lights.add(light_quad);
	lights.add(light_sphere);

	auto mesh_mat = make_shared<lambertian>(color(0.8, 0.7, 0.6));
	auto glass_mat = make_shared<dielectric>(1.5);
	auto purple_metal = make_shared<metal>(color(.65, .05, .65), 0);
	auto obj = load_obj_fit("assets/armadillo.obj", mesh_mat, 500.0, true, purple_metal);
	// obj = make_shared<rotate_x>(obj, -90);
	// obj = make_shared<rotate_y>(obj, 200);
	// obj = make_shared<rotate_z>(obj, 45);
	obj = make_shared<translate>(obj, vec3(278, 170, 278));
	world.add(obj);

	world = hittable_list(make_shared<bvh_node>(world));

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
	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void suzanne_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(7, 7, 7));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	auto light_quad = make_shared<quad>(point3(113, 554, 127), vec3(330, 0, 0), vec3(0, 0, 305), light);
	world.add(light_quad);
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	// shared_ptr<hittable> light_sphere
	// = make_shared<sphere>(point3(0, 0, 0), 35, make_shared<diffuse_light>(color(15, 15, 15)));
	// light_sphere = make_shared<translate>(light_sphere, vec3(470, 75, 150));
	// world.add(light_sphere);

	hittable_list lights;
	lights.add(light_quad);
	// lights.add(light_sphere);

	auto mesh_mat = make_shared<lambertian>(color(0.8, 0.7, 0.6));
	auto glass_mat = make_shared<dielectric>(1.5);
	auto purple_metal = make_shared<metal>(color(.65, .05, .65), 0);
	auto obj = load_obj_fit("assets/suzanne.obj", mesh_mat, 500.0, true, purple_metal, true, true);
	obj = make_shared<rotate_x>(obj, -35);
	obj = make_shared<rotate_y>(obj, 160);
	// obj = make_shared<rotate_z>(obj, 45);
	obj = make_shared<translate>(obj, vec3(278, 87, 278));
	world.add(obj);

	world = hittable_list(make_shared<bvh_node>(world));

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
	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void suzanne_armadillo_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(7, 7, 7));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	auto light_quad = make_shared<quad>(point3(113, 554, 127), vec3(330, 0, 0), vec3(0, 0, 305), light);
	world.add(light_quad);
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	// shared_ptr<hittable> light_sphere
	// = make_shared<sphere>(point3(0, 0, 0), 35, make_shared<diffuse_light>(color(15, 15, 15)));
	// light_sphere = make_shared<translate>(light_sphere, vec3(470, 75, 150));
	// world.add(light_sphere);

	hittable_list lights;
	lights.add(light_quad);
	// lights.add(light_sphere);

	auto mesh_mat = make_shared<lambertian>(color(0.8, 0.7, 0.6));
	auto glass_mat = make_shared<dielectric>(1.5);
	auto purple_metal = make_shared<metal>(color(.65, .05, .65), 0);
	auto suz = load_obj_fit("assets/suzanne.obj", mesh_mat, 300.0, true, purple_metal, true, true);
	suz = make_shared<rotate_x>(suz, -35);
	suz = make_shared<rotate_y>(suz, 160);
	// suz = make_shared<rotate_z>(suz, 45);
	suz = make_shared<translate>(suz, vec3(180, 87, 120));
	world.add(suz);

	auto arm = load_obj_fit("assets/armadillo.obj", mesh_mat, 500.0, true, glass_mat);
	// arm = make_shared<rotate_x>(arm, -90);
	arm = make_shared<rotate_y>(arm, 45);
	// arm = make_shared<rotate_z>(arm, 45);
	arm = make_shared<translate>(arm, vec3(390, 170, 300));
	world.add(arm);

	world = hittable_list(make_shared<bvh_node>(world));

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
	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void adult_link_mesh(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	hittable_list world;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(8, 8, 8));

	world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
	auto light_quad = make_shared<quad>(point3(113, 554, 127), vec3(330, 0, 0), vec3(0, 0, 305), light);
	world.add(light_quad);
	world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
	world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
	world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

	shared_ptr<hittable> light_sphere
		= make_shared<sphere>(point3(0, 0, 0), 15, make_shared<diffuse_light>(color(15, 15, 15)));
	light_sphere = make_shared<translate>(light_sphere, vec3(180, 420, 150));
	world.add(light_sphere);

	shared_ptr<hittable> light_sphere_1
		= make_shared<sphere>(point3(0, 0, 0), 5, make_shared<diffuse_light>(color(0.02, 1.2, 1.6)));
	light_sphere_1 = make_shared<translate>(light_sphere_1, vec3(160, 380, 150));
	world.add(light_sphere_1);

	shared_ptr<hittable> light_sphere_2
		= make_shared<sphere>(point3(0, 0, 0), 2, make_shared<diffuse_light>(color(0.02, 1.2, 1.6)));
	light_sphere_2 = make_shared<translate>(light_sphere_2, vec3(170, 350, 150));
	world.add(light_sphere_2);

	shared_ptr<hittable> light_sphere_3
		= make_shared<sphere>(point3(0, 0, 0), 1, make_shared<diffuse_light>(color(0.02, 1.2, 1.6)));
	light_sphere_3 = make_shared<translate>(light_sphere_3, vec3(180, 390, 150));
	world.add(light_sphere_3);

	shared_ptr<hittable> volume_sphere
		= make_shared<sphere>(point3(0, 0, 0), 25, make_shared<diffuse_light>(color(0.02, 1.2, 1.6)));
	volume_sphere = make_shared<translate>(volume_sphere, vec3(180, 420, 150));
	world.add(make_shared<constant_medium>(volume_sphere, 0.02, color(0.02, 0.5, 0.6)));

	hittable_list lights;
	lights.add(light_quad);
	lights.add(light_sphere);
	lights.add(light_sphere_1);
	lights.add(light_sphere_2);
	lights.add(light_sphere_3);
	lights.add(volume_sphere);

	auto mesh_mat = make_shared<lambertian>(color(0.8, 0.7, 0.6));
	auto glass_mat = make_shared<dielectric>(1.5);
	auto purple_metal = make_shared<metal>(color(.65, .05, .65), 0);
	auto obj = load_obj_fit("assets/adult_link/Untitled.obj", mesh_mat, 500.0, true, nullptr, true, true);
	// obj = make_shared<rotate_x>(obj, -35);
	obj = make_shared<rotate_y>(obj, 180);
	// obj = make_shared<rotate_z>(obj, 45);
	obj = make_shared<translate>(obj, vec3(278, 230, 278));
	world.add(obj);

	world = hittable_list(make_shared<bvh_node>(world));

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
	cam.single_thread = single_thread;
	cam.render(world, lights);
}

void suzanne_glass_spin(int image_width, int samples_per_pixel, int max_depth, bool single_thread) {
	auto glass_mat = make_shared<dielectric>(1.5);

	// Load and tilt the mesh once; each frame re-wraps with a new rotate_y.
	shared_ptr<hittable> base_mesh
		= load_obj_fit("assets/suzanne.obj", glass_mat, 350.0, true, nullptr, true, true);
	base_mesh = make_shared<rotate_x>(base_mesh, -20);

	for (int deg = 0; deg < 360; deg++) {
		hittable_list world;

		auto red = make_shared<lambertian>(color(.65, .05, .05));
		auto white = make_shared<lambertian>(color(.73, .73, .73));
		auto green = make_shared<lambertian>(color(.12, .45, .15));
		auto light = make_shared<diffuse_light>(color(10, 10, 10));

		world.add(make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
		world.add(make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
		auto light_quad
			= make_shared<quad>(point3(113, 554, 127), vec3(330, 0, 0), vec3(0, 0, 305), light);
		world.add(light_quad);
		world.add(make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
		world.add(make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
		world.add(make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

		hittable_list lights;
		lights.add(light_quad);

		shared_ptr<hittable> obj = make_shared<rotate_y>(base_mesh, static_cast<double>(deg));
		obj = make_shared<translate>(obj, vec3(278, 278, 278));
		world.add(obj);

		world = hittable_list(make_shared<bvh_node>(world));

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
		cam.single_thread = single_thread;

		char frame_name[32];
		std::snprintf(frame_name, sizeof(frame_name), "suzanne_glass_spin_%03d.png", deg);
		std::clog << "Frame " << deg << "/359\n";
		cam.render(world, lights, frame_name, true);
	}
}
