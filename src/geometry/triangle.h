#pragma once

#include "quad.h"
#include <array>

// A triangle defined by three vertices v0, v1, v2.
//
// Internally this is a quad with corner Q=v0 and edge vectors u=v1-v0, v=v2-v0.
// That sets up a parallelogram spanning the same plane. Any point on the plane
// can be written as Q + α·u + β·v. The full parallelogram is 0≤α≤1, 0≤β≤1;
// the triangle is the lower-left half: α≥0, β≥0, α+β≤1. The constraint α+β≤1
// is the diagonal edge from v1 (α=1,β=0) to v2 (α=0,β=1). Overriding
// is_interior() to enforce this is the only change needed — quad handles all
// the plane intersection and normal setup.
class triangle : public quad {
  public:
    // Flat shading, no UV data — rec.u/v will be raw barycentric coords.
    triangle(const point3& v0, const point3& v1, const point3& v2, shared_ptr<material> mat)
        : quad(v0, v1 - v0, v2 - v0, mat) {}

    // Flat shading + UV interpolation (OBJ has vt but no vn).
    triangle(const point3& v0, const point3& v1, const point3& v2,
             const std::array<double, 2>& uv0, const std::array<double, 2>& uv1,
             const std::array<double, 2>& uv2, shared_ptr<material> mat)
        : quad(v0, v1 - v0, v2 - v0, mat),
          uv0(uv0), uv1(uv1), uv2(uv2),
          has_uvs(true) {}

    // Smooth shading + UV interpolation. n0/n1/n2 are per-vertex normals
    // and uv0/uv1/uv2 are texture coordinates at each vertex, both from the OBJ.
    triangle(const point3& v0, const point3& v1, const point3& v2,
             const vec3& n0, const vec3& n1, const vec3& n2,
             const std::array<double, 2>& uv0, const std::array<double, 2>& uv1,
             const std::array<double, 2>& uv2, shared_ptr<material> mat)
        : quad(v0, v1 - v0, v2 - v0, mat),
          n0(n0), n1(n1), n2(n2),
          uv0(uv0), uv1(uv1), uv2(uv2),
          has_uvs(true), has_normals(true) {}

    // Restrict quad's parallelogram interior test to the triangle half-space.
    // α and β are the parallelogram coordinates of the hit point (set by quad::hit).
    // The barycentric weights of v0, v1, v2 at this point are (1−α−β), α, β.
    bool is_interior(double a, double b, hit_record& rec) const override {
        if (a < 0 || b < 0 || a + b > 1)
            return false;
        // Store α and β in rec so hit() can read them for attribute interpolation.
        rec.u = a;
        rec.v = b;
        return true;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!quad::hit(r, ray_t, rec))
            return false;
        double a = rec.u, b = rec.v;
        if (has_normals) {
            // Barycentric weights: w0=(1−α−β) for v0, w1=α for v1, w2=β for v2.
            // Interpolating vertex normals approximates the smooth normal of whatever
            // curved surface the mesh is sampling — equivalent to Phong shading in
            // rasterization, but evaluated per ray hit instead of per fragment.
            vec3 interp_n = unit_vector((1 - a - b) * n0 + a * n1 + b * n2);
            rec.set_face_normal(r, interp_n);
        }
        if (has_uvs) {
            // UV coordinates are interpolated with the same barycentric weights so
            // textures map correctly across the triangle face.
            rec.u = (1 - a - b) * uv0[0] + a * uv1[0] + b * uv2[0];
            rec.v = (1 - a - b) * uv0[1] + a * uv1[1] + b * uv2[1];
        }
        return true;
    }

  private:
    vec3 n0{}, n1{}, n2{};
    std::array<double, 2> uv0{}, uv1{}, uv2{};
    bool has_uvs     = false;
    bool has_normals = false;
};
