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
  float dx;
  float dy;
  float dz;
};

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
float lerp(float a, float b, float t) {
  // TODO: see if C++20 does any optimizations for this
  return a + t * (b - a);
}

float vector_length(const Vector& vec) {
  return std::sqrt(vec.dx * vec.dx + vec.dy * vec.dy + vec.dz * vec.dz);
}

// TODO: would a cache help here? e.g. memoization
Color ray_color(const Vector& vec) {
  // 0 < t < 1
  float t = vec.dy / vector_length(vec);

  int red = CMAX * lerp(0.5, 1.0, t);
  int green = CMAX * lerp(0.7, 1.0, t);
  int blue = CMAX;

  return Color{red, green, blue};
}

int main () {
  // image
  float aspect_ratio = 16.0 / 9.0;
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
  float viewport_height = 4.0;
  float viewport_width = viewport_height * aspect_ratio;
  float focal_length = 1.0;

  const std::size_t log_rows = 4;
  std::vector<std::string> log_data;
  std::queue<std::string, std::vector<std::string>> log_queue(log_data);
  Logger logger(log_rows);

  std::ofstream outfile("test.ppm", std::ios::out);
  init_ppm(outfile, width, height);

  // i and j represent one pixel each along the +x and +y axes, respectively.
  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      float x = lerp(0, viewport_width, i / float(width));
      float y = lerp(0, viewport_height, j / float(height));
      float z = focal_length;

      Vector ray{x, y, z};

      Color color = ray_color(ray);

      write_pixel(outfile, color);

      /* std::string data = "Row: " + std::to_string(j) + ", Col: " + std::to_string(i); */
      /* push(log_data, data, log_rows); */
    }
    /* logger.log(log_data); */
  }
}
