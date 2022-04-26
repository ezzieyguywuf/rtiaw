#ifndef RTIAW_UTIL_HEADER
#define RTIAW_UTIL_HEADER

#include <iostream>
#include <ostream>
#include <vector>

#include "vector.h"

namespace rtiaw {
// The maximum value for a given color channel
constexpr int CMAX = 255;

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

struct Ray {
  Vector origin;
  Vector direction;

  Vector at(double t) const {
    return origin + t * direction;
  }
};

struct Color {
  int red = 0;
  int green = 0;
  int blue = 0;

  Color(double r, double g, double b) : red(CMAX * r), green(CMAX * g), blue(CMAX * b) {}
};

std::ostream& operator<<(std::ostream& out, const Color& color) {
  out << "Color{r: "<< color.red
          << ", g: " << color.green
          << ", b: " << color.blue << "}";

  return out;
}

// Initialize a "Portable Pixmap" file in "P3" mode, i.e. ASCII input with full
// color support
void init_ppm(std::ostream& out, int width, int height) {
  out << "P3\n" << width << " " << height << '\n' << CMAX << '\n';
}

// Every pixel will be on its own line
void write_pixel(std::ostream& out, const Color& color) {
  out << color.red << ' ' << color.green << ' ' << color.blue << '\n';
}

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

}
#endif // RTIAW_UTIL_HEADER
