#include <iostream>
#include <fstream>
#include <functional>
#include <optional>
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
  // image
  const double aspect_ratio = 16.0 / 9.0;
  const int height = 711;
  /* double aspect_ratio = 1.0; */
  /* int height = 2; */
  const int width = height * aspect_ratio;

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
  double focal_length = 1.0;

  /* const std::size_t log_rows = 10; */
  /* std::vector<std::string> log_data; */
  /* std::queue<std::string, std::vector<std::string>> log_queue(log_data); */
  /* rtiaw::Logger logger(log_rows); */

  std::ofstream outfile("test.ppm", std::ios::out);
  std::ofstream logfile("log.txt", std::ios::out);
  rtiaw::init_ppm(outfile, width, height);

  rtiaw::Sphere sphere{rtiaw::Vector{0.0, 0.0, 1.0}, 0.5};
  logfile << "width: " << width
          << ", height: " << height
          << ", aspect_ratio: " << aspect_ratio <<'\n';
  logfile << "viewport_width: " << viewport_width
          << ", viewport_height: " << viewport_height << '\n';
  logfile << "Sphere center: " << sphere.center << ", rad: " << sphere.radius<< '\n';

  // i and j represent one pixel each along the +x and +y axes, respectively.
  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      double x = rtiaw::lerp(-viewport_width, viewport_width, i / double(width - 1));
      double y = rtiaw::lerp(-viewport_height, viewport_height, j / double(height));
      double z = focal_length;

      rtiaw::Ray ray{/*origin=*/{0, 0, -1.0}, /*direction=*/{x, y, z}};

      /* logfile << "Row: " << j << ", Col: " << i << ", " << ray; */
      /* logfile << "  x: " << x << ", y: " << y << ", z: " << z << '\n'; */
      const rtiaw::Object& obj = sphere;
      if (std::optional<double> t = obj.check_hit(ray); t.has_value()) {
        // normalize to (0, 1) instead of (-1, 1)
        rtiaw::Vector normal =  0.5 * (rtiaw::Vector{1, 1, 1} + unit_vector(ray.at(*t) - sphere.center));
        /* logfile << "  t: " << t << '\n'; */
        /* logfile << "  ray.at(t): " << ray.at(t) << '\n'; */
        /* logfile << "  normal: " << normal << '\n'; */
        /* logfile << "  normal length: " << vector_length(normal) << '\n'; */
        rtiaw::write_pixel(outfile, {normal.dx, normal.dy, normal.dz});
        /* logfile << "  HIT!\n"; */
      } else {
        rtiaw::Color color = ray_color(ray);
        rtiaw::write_pixel(outfile, color);
        /* logfile << "  MISS!\n"; */
      }

      /* std::ostringstream data; */
      /* data << "Row: " << j << ", Col: " << i << ", " << ray; */
      /* push(log_data, data.str(), log_rows); */
      /* logger.log(log_data); */
    }
  }
}
