#pragma once

#include "quad.h"
#include <array>

class triangle : public quad {
  public:
    triangle(const point3& v0, const point3& v1, const point3& v2, shared_ptr<material> mat)
        : quad(v0, v1 - v0, v2 - v0, mat) {}

    triangle(const point3& v0, const point3& v1, const point3& v2,
             const vec3& n0, const vec3& n1, const vec3& n2,
             const std::array<double, 2>& uv0, const std::array<double, 2>& uv1,
             const std::array<double, 2>& uv2, shared_ptr<material> mat)
        : quad(v0, v1 - v0, v2 - v0, mat),
          n0(n0), n1(n1), n2(n2),
          uv0(uv0), uv1(uv1), uv2(uv2),
          has_attribs(true) {}

    bool is_interior(double a, double b, hit_record& rec) const override {
        if (a < 0 || b < 0 || a + b > 1)
            return false;
        rec.u = a;
        rec.v = b;
        return true;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!quad::hit(r, ray_t, rec))
            return false;
        if (has_attribs) {
            double a = rec.u, b = rec.v;
            vec3 interp_n = unit_vector((1 - a - b) * n0 + a * n1 + b * n2);
            rec.set_face_normal(r, interp_n);
            rec.u = (1 - a - b) * uv0[0] + a * uv1[0] + b * uv2[0];
            rec.v = (1 - a - b) * uv0[1] + a * uv1[1] + b * uv2[1];
        }
        return true;
    }

  private:
    vec3 n0{}, n1{}, n2{};
    std::array<double, 2> uv0{}, uv1{}, uv2{};
    bool has_attribs = false;
};
