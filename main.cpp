#include <chrono>
#include <cmath>
#include <iostream>
#include <fstream>
#include <functional>
#include <optional>
#include <queue>
#include <string>
#include <sstream>
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

std::ostream& operator<<(std::ostream& out, const Vector& vec) {
  out << "{" << vec.dx << ", " << vec.dy << ", " << vec.dz << "}";
  return out;
}


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

std::ostream& operator<<(std::ostream& out, const Ray& ray) {
  out << "Ray{origin: " << ray.origin << ", " 
      <<     "direction:" << ray.direction << "}";
  return out;
}

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
bool hit_sphere(const Ray& ray, const Sphere& sphere, std::optional<std::reference_wrapper<std::ostream>> ostream = std::nullopt) {
  // (A - C) in equations above
  Vector ca{ray.origin.dx - sphere.center.dx,
            ray.origin.dy - sphere.center.dy,
            ray.origin.dz - sphere.center.dz};
  double a = dot(ray.direction, ray.direction);
  double b = -2 * dot(ca, ray.direction);
  double c = dot(ca, ca) - (sphere.radius * sphere.radius);

  if (ostream) {
    ostream.value().get() << "  ca: " << ca << '\n'
             << "    a: " << a << '\n'
             << "    b: " << b << '\n'
             << "    c: " << c << '\n';
  }

  return b*b - 4*a*c > 0;
}

// TODO: would a cache help here? e.g. memoization
// TODO: add origin (for moving the camera later)
Color ray_color(const Ray& ray, std::optional<std::reference_wrapper<std::ostream>> ostream = std::nullopt) {
  // 0 < t < 1
  double t = 0.5 * ray.direction.dy / vector_length(ray.direction);

  if (ostream) {
    ostream.value().get() << "  for vector: " << ray.direction << ", length: " << vector_length(ray.direction) << '\n';
  }

  int red = CMAX * lerp(0.5, 1.0, t);
  int green = CMAX * lerp(0.7, 1.0, t);
  int blue = CMAX;

  return Color{red, green, blue};
}

int main () {
  // image
  const double aspect_ratio = 16.0 / 9.0;
  const int height = 600;
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
  /* Logger logger(log_rows); */

  std::ofstream outfile("test.ppm", std::ios::out);
  std::ofstream logfile("log.txt", std::ios::out);
  init_ppm(outfile, width, height);

  Sphere sphere{Vector{0.0, 0.0, 1.0}, 0.5};
  logfile << "width: " << width
          << ", height: " << height
          << ", aspect_ratio: " << aspect_ratio <<'\n';
  logfile << "viewport_width: " << viewport_width
          << ", viewport_height: " << viewport_height << '\n';
  logfile << "Sphere center: " << sphere.center << ", rad: " << sphere.radius<< '\n';

  // i and j represent one pixel each along the +x and +y axes, respectively.
  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      double x = lerp(-viewport_width, viewport_width, i / double(width - 1));
      double y = lerp(-viewport_height, viewport_height, j / double(height));
      double z = focal_length;

      Ray ray{/*origin=*/{0, 0, 0.0}, /*direction=*/{x, y, z}};

      /* logfile << "Row: " << j << ", Col: " << i << ", " << ray; */
      /* logfile << "  x: " << x << ", y: " << y << ", z: " << z << '\n'; */
      if (hit_sphere(ray, sphere)) {
        write_pixel(outfile, {CMAX, 0, 0});
        /* logfile << "  HIT!\n"; */
      } else {
        Color color = ray_color(ray);
        write_pixel(outfile, color);
        /* logfile << "  MISS!\n"; */
      }

      /* std::ostringstream data; */
      /* data << "Row: " << j << ", Col: " << i << ", " << ray; */
      /* push(log_data, data.str(), log_rows); */
      /* logger.log(log_data); */
    }
  }
}
