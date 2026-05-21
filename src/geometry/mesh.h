#pragma once

#include "geometry/bvh.h"
#include "geometry/hittable_list.h"
#include "geometry/triangle.h"
#include "materials/material.h"
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Parses a single face vertex token of the form v, v/vt, v//vn, or v/vt/vn.
// Returns {vi, ti, ni} with 0-based indices; -1 means absent.
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
        // v/vt/vn or v//vn
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

// Loads an OBJ file and returns a BVH-accelerated triangle mesh.
// All faces are assigned mat. Smooth normals and UV coordinates are used
// when the OBJ provides vn and vt data.
inline shared_ptr<hittable> load_obj(const std::string& path, shared_ptr<material> mat) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "load_obj: cannot open " << path << "\n";
        return make_shared<hittable_list>();
    }

    std::vector<point3> verts;
    std::vector<vec3> normals;
    std::vector<std::array<double, 2>> texcoords;
    hittable_list mesh;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "v") {
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
            // Fan triangulation from vertex 0
            for (int i = 1; i + 1 < (int)face.size(); i++) {
                auto& f0 = face[0];
                auto& f1 = face[i];
                auto& f2 = face[i + 1];

                const point3& v0 = verts[f0[0]];
                const point3& v1 = verts[f1[0]];
                const point3& v2 = verts[f2[0]];

                bool has_n = f0[2] >= 0 && f1[2] >= 0 && f2[2] >= 0;
                bool has_uv = f0[1] >= 0 && f1[1] >= 0 && f2[1] >= 0;

                if (has_n) {
                    std::array<double, 2> uv0 = has_uv ? texcoords[f0[1]] : std::array<double,2>{0,0};
                    std::array<double, 2> uv1 = has_uv ? texcoords[f1[1]] : std::array<double,2>{0,0};
                    std::array<double, 2> uv2 = has_uv ? texcoords[f2[1]] : std::array<double,2>{0,0};
                    mesh.add(make_shared<triangle>(
                        v0, v1, v2,
                        normals[f0[2]], normals[f1[2]], normals[f2[2]],
                        uv0, uv1, uv2, mat));
                } else {
                    mesh.add(make_shared<triangle>(v0, v1, v2, mat));
                }
            }
        }
    }

    std::clog << "load_obj: loaded " << mesh.objects.size() << " triangles from " << path << "\n";
    return make_shared<bvh_node>(mesh);
}

// Loads an OBJ and uniformly scales it so its longest axis fits within target_size.
inline shared_ptr<hittable> load_obj_fit(const std::string& path, shared_ptr<material> mat,
                                          double target_size) {
    auto mesh = load_obj(path, mat);
    auto b = mesh->bounding_box();
    double largest = std::max({b.x.size(), b.y.size(), b.z.size()});
    if (largest < 1e-8)
        return mesh;
    return make_shared<scale>(mesh, target_size / largest);
}
