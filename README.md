# Software Ray Tracer

A C++ Monte Carlo path tracer that simulates physically-based light transport to produce photorealistic renders.

<p align="center">
  <img src="renders/portfolio1.png" alt="Cornell Box — 10000 SPP" width="600"/>
</p>

---

## Features

- **Physically-Based Materials** — Lambertian diffuse, specular metal with Schlick–Fresnel approximation, dielectric refraction (Snell's Law) with total internal reflection
- **BVH Acceleration** — Bounding Volume Hierarchy with AABB slab-method ray tests; reduces scene traversal from O(n) to O(log n)
- **Multithreading** — Tile-based parallel rendering via Intel TBB; ~15× speedup over sequential on a multi-core CPU
- **Optical Effects** — Depth-of-field (defocus disk sampling), motion blur (time-parameterized intersection), area lighting
- **Procedural Textures** — Perlin noise with trilinear interpolation, Hermite smoothing, and multi-octave turbulence
- **Image Textures** — UV-mapped JPG/PNG via stb_image (earth, moon, mars surfaces)
- **Geometry** — Spheres, axis-aligned parallelograms, composite box primitive
- **Anti-aliasing** — Multi-sample per-pixel with random jitter

---

## Gallery

<table>
  <tr>
    <td align="center">
      <img src="renders/portfolio2.png" width="380"/><br/>
      <sub><b>Earth, Moon & Mars</b> — Spherical UV mapping with texture data. 10000 SPP, 1600x900</sub>
    </td>
  </tr>
</table>

---

## Performance

Benchmarks on the Cornell Box scene (600×600). Multithreading uses Intel TBB for tile-based parallelism.

| Configuration | SPP | Render Time |
|---|---|---|
| Single-threaded | 100 | 22m 00s |
| Multi-threaded (TBB) | 100 | 1m 25s |
| Multi-threaded (TBB) | 200 | 2m 45s |

**~15.5× speedup** from multithreading at 100 SPP.

---

## Build

**Dependencies:** CMake ≥ 3.10, C++17 compiler, Intel TBB

```bash
cmake -B build
cmake --build build
./build/raytracing > image.ppm
```

---

## References

- Peter Shirley, Trevor David Black, Steve Hollasch — [*Ray Tracing in One Weekend*](https://raytracing.github.io/books/RayTracingInOneWeekend.html)
- Peter Shirley, Trevor David Black, Steve Hollasch — [*Ray Tracing: The Next Week*](https://raytracing.github.io/books/RayTracingTheNextWeek.html)
