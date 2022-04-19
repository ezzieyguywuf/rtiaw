#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <vector>

// The maximum value for a given color channel
constexpr int CMAX = 255;

// Initialize a "Portable Pixmap" file in "P3" mode, i.e. ASCII input with full
// color support
void init_ppm(std::ostream& out, int width, int height) {
  out << "P3\n" << width << " " << height << '\n' << CMAX << '\n';
}

struct Color {
  int red = 0;
  int green = 0; 
  int blue = 0;
};

struct Vector {
  double dx;
  double dy;
  double dz;

  Vector operator*(int val) const {
    return {dx * val, dy * val, dz * val};
  }
};

Vector operator* (int val, const Vector& vec) {
  return vec * val;
}

// Every pixel will be on its own line
void write_pixel(std::ostream& out, const Color& color) {
  out << color.red << ' ' << color.green << ' ' << color.blue << '\n';
}

class Logger {
  public:
    Logger(std::size_t rows = 10) : rows_(rows) {
      std::cout << std::string(rows_, '\n') << std::flush;
    }

    // Logs the data std::cout, using ANSI escape sequences to reuse the same
    // area of the terminal rather than scrolling
    void log(const std::vector<std::string>& data) {
      // Move cursor up by rows_
      std::cout << "\033[" << rows_ << "A";
      for (const std::string& d : data) {
        std::cout << d << '\n';
      }

      if (data.size() < rows_) {
        std::cout << std::string ( rows_ - data.size(), '\n' );
      }

      std::cout << std::flush;
    }


  private:
    std::size_t rows_;
};

// A very rudimentary "queue" - when the vector reaches the target size, the
// first element will be removed, everything will be shifted "up", and finally
// the target item is added to the back
void push(
  std::vector<std::string>& data, const std::string& item, std::size_t n = 10) {
  if (data.size() < n) {
    data.push_back(item);
    return;
  }

  for (std::size_t i = 0; i < n - 1; ++i) {
    data[i] = data[i + 1];
  }
  data[n - 1] = item;
}

// The linear interpolation of t between a and b
double lerp(double a, double b, double t) {
  // TODO: see if C++20 does any optimizations for this
  return a + t * (b - a);
}

double vector_length(const Vector& vec) {
  return std::sqrt(vec.dx * vec.dx + vec.dy * vec.dy + vec.dz * vec.dz);
}

double dot(const Vector& u, const Vector& v) {
  return u.dx * v.dx + u.dy * v.dy + u.dz * v.dz;
}

struct Sphere {
  Vector center;
  double radius;
};

struct Ray {
  Vector origin;
  Vector direction;
};

// General equation for a sphere: (x - Cx)² + (y - Cy)² + (z - Cz)² = r²
// in vector form, C = {Cx, Cy, Cz}, P = {x, y, z}, so (P - C) · (P - C) = r²
//
// This can be solved using the quadratic formula but we don't need a solution -
// we only need to know the number of roots. As described on wikipedia (or via
// intuition), we can determine the number of roots based on the "discriminant",
// or the part inside the square root:
//
//   discriminant > 0, 2 roots (hit)
//   discriminant = 0, 1 root (hit) - +/- 0 yields a single root
//   discriminant < 0, 0 root (miss) - this would be a negative square root
//
//   discriminant = b² - 4·a·c
//
// We need to expand P to P = A + t·B, which is our equation for our ray.
//   A = origin of ray
//   B = direction of ray
//
// (P - C) · (P - C) = (A + t·B - C) · (A + t·B - C)
//                   = (B·B)t² - 2(B·(A - C))t + ((A-C)·(A-C) - r²) = 0
bool hit_sphere(const Ray& ray, const Sphere& sphere) {
  // (A - C) in equations above
  Vector ca{ray.origin.dx - sphere.center.dx,
            ray.origin.dy - sphere.center.dy,
            ray.origin.dz - sphere.center.dz};
  double a = dot(ray.direction, ray.direction);
  double b = 2 * dot(ca, ray.direction);
  double c = dot(ca, ca) - (sphere.radius * sphere.radius);

  return b*b - 4*a*c > 0;
}

// TODO: would a cache help here? e.g. memoization
// TODO: add origin (for moving the camera later)
Color ray_color(const Ray& ray) {
  // 0 < t < 1
  double t = ray.direction.dy / vector_length(ray.direction);

  int red = CMAX * lerp(0.5, 1.0, t);
  int green = CMAX * lerp(0.7, 1.0, t);
  int blue = CMAX;

  return Color{red, green, blue};
}

int main () {
  // image
  double aspect_ratio = 16.0 / 9.0;
  const int height = 600;
  const int width = height * aspect_ratio;

  // Camera
  //
  // Our viewport is scaled down to 0 < y < 4
  // x is fixed by the apsect ratio, which is width / height, or x / y
  // therefor, 0 < x < 4 * 16 / 9
  // or approx. 0 < x < 7.11
  // 
  // x is positive to the right
  // y is positive down
  //
  // Top-left of the screen is x = 0, y = 0
  // bottom-right of the screen is x = width, y = height
  // positive Z is into the screen
  double viewport_height = 4.0;
  double viewport_width = viewport_height * aspect_ratio;
  double focal_length = 1.0;

  const std::size_t log_rows = 4;
  std::vector<std::string> log_data;
  std::queue<std::string, std::vector<std::string>> log_queue(log_data);
  Logger logger(log_rows);

  std::ofstream outfile("test.ppm", std::ios::out);
  init_ppm(outfile, width, height);

  Sphere sphere{Vector{viewport_width/2.0, viewport_height/2.0, focal_length}, 0.5};

  // i and j represent one pixel each along the +x and +y axes, respectively.
  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      double x = lerp(0, viewport_width, i / double(width));
      double y = lerp(0, viewport_height, j / double(height));
      double z = focal_length;

      Ray ray{/*origin=*/{0,0,0}, /*direction=*/{x, y, z}};

      if (hit_sphere(ray, sphere)) {
        write_pixel(outfile, {CMAX, 0, 0});
      } else {
        Color color = ray_color(ray);
        write_pixel(outfile, color);
      }

      /* std::string data = "Row: " + std::to_string(j) + ", Col: " + std::to_string(i); */
      /* push(log_data, data, log_rows); */
    }
    /* logger.log(log_data); */
  }
}
