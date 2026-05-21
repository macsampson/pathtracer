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
static std::array<int, 3> parse_face_token(const std::string& tok,
                                            int nv, int nt, int nn) {
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
load_mtl(const std::string& path, const std::filesystem::path& mtl_dir) {
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
        if (current_name.empty()) return;
        shared_ptr<material> mat;
        if (!current_map_kd.empty()) {
            auto tex_path = mtl_dir / current_map_kd;
            mat = make_shared<lambertian>(
                make_shared<image_texture>(tex_path.string().c_str()));
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
        if (line.empty() || line[0] == '#') continue;
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
// If the OBJ references a .mtl file via `mtllib`, per-material textures are
// loaded automatically from that file. The fallback `mat` is used for faces
// that have no `usemtl` assignment or whose material isn't in the MTL.
//
// The mesh gets its own BVH node, separate from the scene-level BVH. This
// means ray traversal into a large mesh does O(log n) work over just the
// mesh's triangles — the scene BVH never has to hold individual triangles
// as leaves, keeping both BVHs compact.
inline shared_ptr<hittable> load_obj(const std::string& path, shared_ptr<material> fallback_mat) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "load_obj: cannot open " << path << "\n";
        return make_shared<hittable_list>();
    }

    auto obj_dir = std::filesystem::path(path).parent_path();

    std::unordered_map<std::string, shared_ptr<material>> mtl_map;
    shared_ptr<material> current_mat = fallback_mat;

    std::vector<point3> verts;
    std::vector<vec3> normals;
    std::vector<std::array<double, 2>> texcoords;
    hittable_list mesh;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "mtllib") {
            std::string mtl_file;
            ss >> mtl_file;
            mtl_map = load_mtl((obj_dir / mtl_file).string(), obj_dir);
        } else if (token == "usemtl") {
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
            std::vector<std::array<int, 3>> face;
            std::string tok;
            while (ss >> tok)
                face.push_back(parse_face_token(tok, verts.size(),
                                                texcoords.size(), normals.size()));

            // Fan triangulation: anchor at face[0] and walk the remaining vertices.
            // Face (v0, v1, v2, v3, ...) becomes triangles (v0,v1,v2), (v0,v2,v3), ...
            // This is correct for convex polygons. Most OBJ exporters either output
            // triangles directly or produce convex quads, so this covers the common cases.
            for (int i = 1; i + 1 < (int)face.size(); i++) {
                auto& f0 = face[0];
                auto& f1 = face[i];
                auto& f2 = face[i + 1];

                const point3& v0 = verts[f0[0]];
                const point3& v1 = verts[f1[0]];
                const point3& v2 = verts[f2[0]];

                // Only use smooth shading if all three vertices have normals.
                // Mixing flat and interpolated normals within a mesh would cause
                // visible seams, so we fall back to flat if any normal is missing.
                bool has_n  = f0[2] >= 0 && f1[2] >= 0 && f2[2] >= 0;
                bool has_uv = f0[1] >= 0 && f1[1] >= 0 && f2[1] >= 0;

                if (has_n && has_uv) {
                    mesh.add(make_shared<triangle>(
                        v0, v1, v2,
                        normals[f0[2]], normals[f1[2]], normals[f2[2]],
                        texcoords[f0[1]], texcoords[f1[1]], texcoords[f2[1]],
                        current_mat));
                } else if (has_n) {
                    // Normals but no UVs — smooth shading, barycentric UV fallback.
                    // Pass zero UVs; the texture will sample a fixed point.
                    mesh.add(make_shared<triangle>(
                        v0, v1, v2,
                        normals[f0[2]], normals[f1[2]], normals[f2[2]],
                        std::array<double,2>{0,0}, std::array<double,2>{0,0},
                        std::array<double,2>{0,0}, current_mat));
                } else if (has_uv) {
                    // UVs but no normals (e.g. old 3ds Max exports) — flat shading,
                    // interpolated UVs. This is the common case for textured OBJs
                    // without explicit vertex normals.
                    mesh.add(make_shared<triangle>(
                        v0, v1, v2,
                        texcoords[f0[1]], texcoords[f1[1]], texcoords[f2[1]],
                        current_mat));
                } else {
                    mesh.add(make_shared<triangle>(v0, v1, v2, current_mat));
                }
            }
        }
    }

    std::clog << "load_obj: loaded " << mesh.objects.size() << " triangles from " << path << "\n";
    return make_shared<bvh_node>(mesh);
}

// Loads an OBJ and uniformly scales it so its longest axis fits within target_size.
// OBJ files have no standard unit — a model exported from Blender in metres and one
// exported in millimetres look identical in the file. This helper measures the mesh's
// bounding box after loading and computes the scale factor automatically.
inline shared_ptr<hittable> load_obj_fit(const std::string& path, shared_ptr<material> fallback_mat,
                                          double target_size) {
    auto mesh = load_obj(path, fallback_mat);
    auto b = mesh->bounding_box();
    double largest = std::max({b.x.size(), b.y.size(), b.z.size()});
    if (largest < 1e-8)
        return mesh;
    return make_shared<scale>(mesh, target_size / largest);
}
