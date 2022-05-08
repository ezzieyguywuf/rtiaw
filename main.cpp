#include <iostream>
#include <fstream>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <sstream>
#include <vector>

#include "objects.h"
#include "util.h"
#include "vector.h"

// TODO: would a cache help here? e.g. memoization
// TODO: add origin (for moving the camera later)
rtiaw::Color ray_color(const rtiaw::Ray& ray, std::optional<std::reference_wrapper<std::ostream>> ostream = std::nullopt) {
  // 0 < t < 1
  double t = 0.5 * ray.direction.dy / vector_length(ray.direction);

  if (ostream) {
    ostream.value().get() << "  for vector: " << ray.direction << ", length: " << vector_length(ray.direction) << '\n';
  }

  double red = rtiaw::lerp(0.5, 1.0, t);
  double green = rtiaw::lerp(0.7, 1.0, t);
  double blue = 1.0;

  return rtiaw::Color{red, green, blue};
}

int main () {
  static std::uniform_real_distribution<double> distribution(-0.5, 0.5);
  static std::mt19937 generator;

  // image
  const double aspect_ratio = 16.0 / 9.0;
  const int height = 711;
  // const int height = 4;
  /* double aspect_ratio = 1.0; */
  /* int height = 2; */
  const int width = height * aspect_ratio;
  const int samples_per_pixel = 100;

  // Camera
  //
  // Our viewport is scaled down to  -2 < y < 2
  // x is fixed by the apsect ratio, which is width / height, or x / y
  // therefor, -2 * 16/9 < x < 2 * 16/9
  // or approx. -3.55 < x < 3.55
  //
  // x is positive to the right
  // y is positive down
  //
  // Top-left of the screen is x = -width/2, y = -height/2
  // bottom-right of the screen is x = width/2, y = height/2
  // positive Z is into the screen
  double viewport_height = 2.0;
  double viewport_width = viewport_height * aspect_ratio;
  rtiaw::Camera cam(viewport_height, viewport_width, /*focal_length=*/1.0);

  rtiaw::PpmWriter writer("output.ppm", width, height);
  std::ofstream logfile("log.txt", std::ios::out);
  rtiaw::Logger logger;

  // Objects in the world
  std::vector<std::reference_wrapper<rtiaw::Object>> objects;
  rtiaw::Sphere sphere1{rtiaw::Vector{0.0, 0.0, 1.0}, 0.5};
  rtiaw::Sphere sphere2{rtiaw::Vector{0.0, 100.6, 1.0}, 100};
  objects.push_back(sphere1);
  objects.push_back(sphere2);

  logfile << "width: " << width
          << ", height: " << height
          << ", aspect_ratio: " << aspect_ratio <<'\n';
  logfile << "viewport_width: " << viewport_width
          << ", viewport_height: " << viewport_height << '\n';

  // i and j represent one pixel each along the +x and +y axes, respectively.
  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      bool hit = false;
      rtiaw::Color color{0, 0, 0};
      for (int sample = 0; sample < samples_per_pixel; ++sample) {
        double u = (i + distribution(generator)) / double(width - 1);
        double v = (j + distribution(generator)) / double(height);
        rtiaw::Ray ray = cam.ray_at(u, v);

        if (sample == 0 ) {
          color = ray_color(ray);
        }

        double max_t = std::numeric_limits<double>::infinity();
        for (const rtiaw::Object& obj : objects ) {
          if (std::optional<rtiaw::Object::HitData> hd = obj.check_hit(ray, 0, max_t); hd.has_value()) {
            // normalize to (0, 1) instead of (-1, 1)
            hit = true;
            max_t = hd->t;
            rtiaw::Vector normal = 0.5 * (rtiaw::Vector{1, 1, 1} + hd->normal);
            rtiaw::Color delta(normal.dx, normal.dy, normal.dz);
            color.red += delta.red;
            color.green += delta.green;
            color.blue += delta.blue;
          }      
        }
      }

      if (hit) {
        color.red   /= samples_per_pixel;
        color.green /= samples_per_pixel;
        color.blue  /= samples_per_pixel;
      }

      writer.write_pixel(color, j, i);
    }

    std::stringstream sstream;
    sstream << (float) j / height * 100 << "% complete";
    logger.push(sstream.str());
    logger.print_log();
  }
}
