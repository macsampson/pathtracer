#pragma once

#include "geometry/bvh.h"
#include "geometry/hittable_list.h"
#include "geometry/triangle.h"
#include "materials/material.h"
#include "materials/texture.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Parses one vertex token from an OBJ face line. The four formats are:
//   v        — vertex index only
//   v/vt     — vertex + texture coordinate
//   v//vn    — vertex + normal (no texture)
//   v/vt/vn  — vertex + texture + normal
//
// OBJ indices are 1-based. Negative values are relative to the end of the
// current list (e.g. -1 is the last vertex added so far), so we resolve them
// to 0-based absolute indices here. Missing attributes are returned as -1.
static std::array<int, 3> parse_face_token(const std::string& tok, int nv, int nt, int nn) {
	std::array<int, 3> out = {-1, -1, -1};
	auto slash1 = tok.find('/');
	if (slash1 == std::string::npos) {
		int idx = std::stoi(tok);
		out[0] = idx < 0 ? nv + idx : idx - 1;
		return out;
	}
	auto slash2 = tok.find('/', slash1 + 1);
	{
		int idx = std::stoi(tok.substr(0, slash1));
		out[0] = idx < 0 ? nv + idx : idx - 1;
	}
	if (slash2 == std::string::npos) {
		// v/vt
		std::string ts = tok.substr(slash1 + 1);
		if (!ts.empty()) {
			int idx = std::stoi(ts);
			out[1] = idx < 0 ? nt + idx : idx - 1;
		}
	} else {
		// v/vt/vn or v//vn (empty string between slashes means no texcoord)
		std::string ts = tok.substr(slash1 + 1, slash2 - slash1 - 1);
		std::string ns = tok.substr(slash2 + 1);
		if (!ts.empty()) {
			int idx = std::stoi(ts);
			out[1] = idx < 0 ? nt + idx : idx - 1;
		}
		if (!ns.empty()) {
			int idx = std::stoi(ns);
			out[2] = idx < 0 ? nn + idx : idx - 1;
		}
	}
	return out;
}

// Parses a .mtl file and returns a map from material name to material.
// Only map_Kd (diffuse texture) and Kd (diffuse color) are used — ambient,
// specular, and transparency properties don't apply to a path tracer.
// Texture paths are resolved relative to mtl_dir.
static std::unordered_map<std::string, shared_ptr<material>>
load_mtl(const std::string& path, const std::filesystem::path& mtl_dir, bool mirror_u = false) {
	std::unordered_map<std::string, shared_ptr<material>> mats;
	std::ifstream file(path);
	if (!file) {
		std::cerr << "load_mtl: cannot open " << path << "\n";
		return mats;
	}

	std::string current_name;
	color current_kd(0.8, 0.8, 0.8);
	std::string current_map_kd;

	// Commit the current material and reset state for the next one.
	auto commit = [&]() {
		if (current_name.empty())
			return;
		shared_ptr<material> mat;
		if (!current_map_kd.empty()) {
			auto tex_path = mtl_dir / current_map_kd;
			mat = make_shared<lambertian>(
				make_shared<image_texture>(tex_path.string().c_str(), mirror_u));
		} else {
			mat = make_shared<lambertian>(current_kd);
		}
		mats[current_name] = mat;
		current_name.clear();
		current_kd = color(0.8, 0.8, 0.8);
		current_map_kd.clear();
	};

	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#')
			continue;
		std::istringstream ss(line);
		std::string token;
		ss >> token;
		if (token == "newmtl") {
			commit();
			ss >> current_name;
		} else if (token == "Kd") {
			double r, g, b;
			ss >> r >> g >> b;
			current_kd = color(r, g, b);
		} else if (token == "map_Kd") {
			ss >> current_map_kd;
		}
	}
	commit();
	return mats;
}

// Loads an OBJ file and returns a BVH-accelerated triangle mesh.
//
// If the OBJ references a .mtl file via `mtllib`, per-material textures are
// loaded automatically from that file. The fallback `mat` is used for faces
// that have no `usemtl` assignment or whose material isn't in the MTL.
//
// The mesh gets its own BVH node, separate from the scene-level BVH. This
// means ray traversal into a large mesh does O(log n) work over just the
// mesh's triangles — the scene BVH never has to hold individual triangles
// as leaves, keeping both BVHs compact.
//
// smooth: controls normal shading for the mesh.
//   true  — use `vn` normals from the OBJ if present; if absent, compute
//           area-weighted vertex normals from the mesh geometry (two-pass).
//           Produces smooth curvature across shared edges.
//   false — always use the flat geometric face normal (one per triangle).
//           Produces hard faceted edges regardless of whether `vn` is present.
//
// Area-weighted vertex normal computation: for each vertex, accumulate the
// unnormalized cross products of all triangles that share it, then normalize.
// The cross product magnitude equals 2 × triangle area, so larger triangles
// contribute proportionally more — this is better than plain averaging for
// meshes with non-uniform tessellation and costs nothing extra.
//
// Hard creases: if the model was exported with vertices split at crease edges
// (the standard approach in Blender, Maya, etc.), averaging still produces
// correct hard edges because the two vertex copies accumulate normals from
// disjoint triangle sets.
//
// override_mat: when non-null, all triangles use this material regardless of
// usemtl directives in the OBJ. MTL loading is skipped entirely. Useful for
// rendering a mesh as glass, metal, etc. without caring about the original
// texture assignment.
inline shared_ptr<hittable> load_obj(const std::string& path,
                                     shared_ptr<material> fallback_mat,
                                     bool mirror_u = false,
                                     shared_ptr<material> override_mat = nullptr,
                                     bool smooth = true,
                                     bool center = false) {
	std::ifstream file(path);
	if (!file) {
		std::cerr << "load_obj: cannot open " << path << "\n";
		return make_shared<hittable_list>();
	}

	auto obj_dir = std::filesystem::path(path).parent_path();

	std::unordered_map<std::string, shared_ptr<material>> mtl_map;
	shared_ptr<material> current_mat = override_mat ? override_mat : fallback_mat;

	std::vector<point3> verts;
	std::vector<vec3> normals;
	std::vector<std::array<double, 2>> texcoords;

	// Buffer all face data for the two-pass normal computation path.
	// Each FaceVert holds resolved 0-based indices into the attribute arrays
	// (-1 means the attribute was absent for that vertex).
	struct FaceVert {
		int vi, ti, ni;
	};
	std::vector<std::vector<FaceVert>> faces;
	std::vector<shared_ptr<material>> face_mats;

	// --- First pass: read geometry and buffer faces ---
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#')
			continue;
		std::istringstream ss(line);
		std::string token;
		ss >> token;

		if (!override_mat && token == "mtllib") {
			std::string mtl_file;
			ss >> mtl_file;
			mtl_map = load_mtl((obj_dir / mtl_file).string(), obj_dir, mirror_u);
		} else if (!override_mat && token == "usemtl") {
			std::string name;
			ss >> name;
			auto it = mtl_map.find(name);
			current_mat = (it != mtl_map.end()) ? it->second : fallback_mat;
		} else if (token == "v") {
			double x, y, z;
			ss >> x >> y >> z;
			verts.push_back(point3(x, y, z));
		} else if (token == "vn") {
			double x, y, z;
			ss >> x >> y >> z;
			normals.push_back(vec3(x, y, z));
		} else if (token == "vt") {
			double u, v;
			ss >> u >> v;
			texcoords.push_back({u, v});
		} else if (token == "f") {
			std::vector<FaceVert> face;
			std::string tok;
			while (ss >> tok) {
				auto idx = parse_face_token(tok, verts.size(), texcoords.size(), normals.size());
				face.push_back({idx[0], idx[1], idx[2]});
			}
			faces.push_back(std::move(face));
			face_mats.push_back(current_mat);
		}
	}

	// --- Compute vertex normals if smooth shading requested and OBJ has none ---
	//
	// Accumulate area-weighted face normals at each vertex position, then normalize.
	// The cross product (e1 × e2) is unnormalized — its magnitude is 2 × area —
	// so each face's contribution is automatically scaled by its area. Larger
	// triangles steer the normal more than tiny slivers. After accumulation,
	// normalize each vertex normal to unit length.
	bool computed_normals = false;
	if (smooth && normals.empty() && !verts.empty()) {
		normals.resize(verts.size(), vec3(0, 0, 0));
		for (auto& face : faces) {
			// Fan triangulation matches the triangle-building loop below.
			for (int i = 1; i + 1 < (int)face.size(); i++) {
				vec3 e1 = verts[face[i].vi] - verts[face[0].vi];
				vec3 e2 = verts[face[i + 1].vi] - verts[face[0].vi];
				vec3 fn = cross(e1, e2); // unnormalized, magnitude = 2 * area
				normals[face[0].vi] += fn;
				normals[face[i].vi] += fn;
				normals[face[i + 1].vi] += fn;
			}
		}
		for (auto& n : normals)
			if (n.length_squared() > 1e-30)
				n = unit_vector(n);
		computed_normals = true;
	}

	// --- Second pass: build triangles ---
	hittable_list mesh;
	for (int fi = 0; fi < (int)faces.size(); fi++) {
		auto& face = faces[fi];
		auto mat = face_mats[fi];

		// Fan triangulation: anchor at face[0] and walk the remaining vertices.
		// Face (v0, v1, v2, v3, ...) becomes triangles (v0,v1,v2), (v0,v2,v3), ...
		// This is correct for convex polygons. Most OBJ exporters either output
		// triangles directly or produce convex quads, so this covers the common cases.
		for (int i = 1; i + 1 < (int)face.size(); i++) {
			auto& f0 = face[0];
			auto& f1 = face[i];
			auto& f2 = face[i + 1];

			const point3& v0 = verts[f0.vi];
			const point3& v1 = verts[f1.vi];
			const point3& v2 = verts[f2.vi];

			bool has_uv = f0.ti >= 0 && f1.ti >= 0 && f2.ti >= 0;

			// Resolve normals:
			//   computed_normals → indexed by vertex position (vi), always present
			//   OBJ vn present   → indexed by face normal index (ni), may be absent
			//   smooth=false     → no normals, flat shading
			bool has_n = false;
			vec3 n0, n1, n2;
			if (smooth) {
				if (computed_normals) {
					// Every vertex has a computed normal — always use smooth shading.
					has_n = true;
					n0 = normals[f0.vi];
					n1 = normals[f1.vi];
					n2 = normals[f2.vi];
				} else {
					// OBJ-supplied normals; only use smooth shading if all three
					// vertices have a normal index (mixing flat and smooth causes seams).
					has_n = f0.ni >= 0 && f1.ni >= 0 && f2.ni >= 0;
					if (has_n) {
						n0 = normals[f0.ni];
						n1 = normals[f1.ni];
						n2 = normals[f2.ni];
					}
				}
			}

			if (has_n && has_uv) {
				mesh.add(make_shared<triangle>(v0, v1, v2, n0, n1, n2,
				                              texcoords[f0.ti], texcoords[f1.ti],
				                              texcoords[f2.ti], mat));
			} else if (has_n) {
				// Normals but no UVs — smooth shading, zero UV fallback.
				mesh.add(make_shared<triangle>(v0, v1, v2, n0, n1, n2,
				                              std::array<double, 2>{0, 0},
				                              std::array<double, 2>{0, 0},
				                              std::array<double, 2>{0, 0}, mat));
			} else if (has_uv) {
				// UVs but no normals — flat shading with interpolated UVs.
				mesh.add(make_shared<triangle>(v0, v1, v2,
				                              texcoords[f0.ti], texcoords[f1.ti],
				                              texcoords[f2.ti], mat));
			} else {
				mesh.add(make_shared<triangle>(v0, v1, v2, mat));
			}
		}
	}

	std::clog << "load_obj: loaded " << mesh.objects.size() << " triangles from " << path
	          << (computed_normals ? " (computed vertex normals)" : "") << "\n";
	shared_ptr<hittable> result = make_shared<bvh_node>(mesh);
	if (center) {
		auto b = result->bounding_box();
		vec3 offset(-(b.x.min + b.x.max) * 0.5, -(b.y.min + b.y.max) * 0.5,
		            -(b.z.min + b.z.max) * 0.5);
		result = make_shared<translate>(result, offset);
	}
	return result;
}

// Loads an OBJ and uniformly scales it so its longest axis fits within target_size.
// OBJ files have no standard unit — a model exported from Blender in metres and one
// exported in millimetres look identical in the file. This helper measures the mesh's
// bounding box after loading and computes the scale factor automatically.
inline shared_ptr<hittable> load_obj_fit(const std::string& path,
                                         shared_ptr<material> fallback_mat,
                                         double target_size,
                                         bool mirror_u = false,
                                         shared_ptr<material> override_mat = nullptr,
                                         bool smooth = true,
                                         bool center = false) {
	auto mesh = load_obj(path, fallback_mat, mirror_u, override_mat, smooth, center);
	auto b = mesh->bounding_box();
	double largest = std::max({b.x.size(), b.y.size(), b.z.size()});
	if (largest < 1e-8)
		return mesh;
	return make_shared<scale>(mesh, target_size / largest);
}
