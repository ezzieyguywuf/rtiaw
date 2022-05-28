#ifndef RTIAW_UTIL_HEADER
#define RTIAW_UTIL_HEADER

#include <algorithm>
#include <fstream>
#include <iomanip>
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
  void push(const std::string &item) {
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
    for (const std::string &d : data_) {
      std::cout << d << '\n';
    }

    if (data_.size() < rows_) {
      std::cout << std::string(rows_ - data_.size(), '\n');
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

  Vector at(double t) const { return origin + t * direction; }
};

struct Color {
  int red = 0;
  int green = 0;
  int blue = 0;

  explicit Color(double r, double g, double b)
      : red(CMAX * r), green(CMAX * g), blue(CMAX * b) {}
  explicit Color(int r, int g, int b) : red(r), green(g), blue(b) {}

  // Writes first red, then green, then blue to the stream. Each write will be
  // an 8-bit byte.
  void write_bytes(std::ostream &out) const {
    char cred = std::clamp(red, 0, 255);
    char cgreen = std::clamp(green, 0, 255);
    char cblue = std::clamp(blue, 0, 255);
    out.write(&cred, sizeof(cred));
    out.write(&cgreen, sizeof(cgreen));
    out.write(&cblue, sizeof(cblue));
  }
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

  Ray ray_at(double u, double v) const {
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

std::ostream &operator<<(std::ostream &out, const Color &color) {
  out << "Color{r: " << color.red << ", g: " << color.green
      << ", b: " << color.blue << "}";

  return out;
}

std::ofstream &operator<<(std::ofstream &out, const Color &color) {
  out << std::setw(3) << color.red << ' ' << std::setw(3) << color.green << ' '
      << std::setw(3) << color.blue << '\n';

  return out;
}

std::fstream &operator<<(std::fstream &out, const Color &color) {
  out << std::setw(3) << color.red << ' ' << std::setw(3) << color.green << ' '
      << std::setw(3) << color.blue << '\n';

  return out;
}

// A "Portable Pixmap" write in "P3" mode, i.e. ASCII input with full color
// support
class PpmWriter {
public:
  // This will overwrite the file if it already exists
  PpmWriter(const std::string &filename, int64_t width, int64_t height,
            const Color &canvas = Color(180, 255, 200))
      : nCol_(width) {
    {
      std::ofstream new_file(filename);
      new_file << "P3\n" << width << " " << height << '\n' << CMAX << '\n';
      offset_ = new_file.tellp();
      new_file << SENTINEL << '\n';

      for (int64_t i = 0; i < width * height; ++i) {
        new_file << canvas;
      }
    }
    file_ = std::fstream(filename);
    std::cout << "offset: " << offset_ << '\n';
  }

  void write_pixel(const Color &color, int64_t row, int64_t col) {
    file_.seekg(offset_);
    std::string check;
    std::getline(file_, check);
    if (check != SENTINEL) {
      std::cerr
          << "Sentinel value does not match - file seems corrupt. NOT WRITING!"
          << '\n';
      std::cerr << "     got: " << check << '\n';
      std::cerr << "  expect: " << SENTINEL << '\n';
      return;
    }

    // TODO: this is pretty gnarly - try to make it less gnarly
    std::fstream::pos_type offset =
        offset_ + std::fstream::pos_type(29) + row * (12 * nCol_) + col * 12;
    std::cout << "for row: " << row << ", col: " << col
              << ", offset: " << offset << '\n';
    file_.seekg(offset);
    file_ << color;
  }

private:
  constexpr static const char *const SENTINEL = "#SENTINAL pixels start below";
  std::fstream file_;
  std::fstream::pos_type offset_;
  std::fstream::pos_type nCol_;
};

// Every pixel will be on its own line
void write_pixel(std::ostream &out, const Color &color) {
  out << color.red << ' ' << color.green << ' ' << color.blue << '\n';
}

} // namespace rtiaw
#endif // RTIAW_UTIL_HEADER
