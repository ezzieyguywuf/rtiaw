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
      data_.reserve(rows_);
      std::cout << std::string(rows_, '\n') << std::flush;
    }

    // A very rudimentary "queue" - when the vector reaches the target size, the
    // first element will be removed, everything will be shifted "up", and finally
    // the target item is added to the back
    void push(const std::string& item) {
      if (data_.size() < rows_) {
        data_.push_back(item);
      } else {
        for (std::size_t i = 0; i < rows_ - 1; ++i) {
          data_[i] = data_[i + 1];
        }
        data_[rows_ - 1] = item;
      }
    }

    // Logs the data std::cout, using ANSI escape sequences to reuse the same
    // area of the terminal rather than scrolling
    void print_log() const {
      // Move cursor up by rows_
      std::cout << "\033[" << rows_ << "A";
      for (const std::string& d : data_) {
        std::cout << d << '\n';
      }

      if (data_.size() < rows_) {
        std::cout << std::string ( rows_ - data_.size(), '\n' );
      }

      std::cout << std::flush;
    }

  private:
    std::vector<std::string> data_;
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

// The linear interpolation of t between a and b
double lerp(double a, double b, double t) {
  // TODO: see if C++20 does any optimizations for this
  return a + t * (b - a);
}

class Camera {
  public:
    Camera(double viewport_height, double viewport_width, double focal_length)
      : viewport_height_(viewport_height), viewport_width_(viewport_width),
        focal_length_(focal_length) {}

    Ray ray_at(double u, double v) {
      double x = rtiaw::lerp(-viewport_width_, viewport_width_, u);
      double y = rtiaw::lerp(-viewport_height_, viewport_height_, v);
      double z = focal_length_;

      return rtiaw::Ray{/*origin=*/location_, /*direction=*/{x, y, z}};
    }

  private:
    Vector location_ = {0, 0, 0};
    double viewport_height_;
    double viewport_width_;
    double focal_length_;
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

}
#endif // RTIAW_UTIL_HEADER
