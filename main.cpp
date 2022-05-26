#include <algorithm>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Clock.hpp>

#include "objects.h"
#include "util.h"
#include "vector.h"

// TODO: would a cache help here? e.g. memoization
// TODO: add origin (for moving the camera later)
rtiaw::Color ray_color(const rtiaw::Ray &ray,
                       std::optional<std::reference_wrapper<std::ostream>>
                           ostream = std::nullopt) {
  // 0 < t < 1
  double t = 0.5 * ray.direction.dy / vector_length(ray.direction);

  if (ostream) {
    ostream.value().get() << "  for vector: " << ray.direction
                          << ", length: " << vector_length(ray.direction)
                          << '\n';
  }

  double red = rtiaw::lerp(0.5, 1.0, t);
  double green = rtiaw::lerp(0.7, 1.0, t);
  double blue = 1.0;

  return rtiaw::Color{red, green, blue};
}

// This will hold enough memory to store all of our pixel data.
class Framebuffer {
public:
  struct Pixel {
    std::size_t row;
    std::size_t col;
    int segment;

    bool operator==(const Pixel &other) const {
      return row == other.row && col == other.col && segment == other.segment;
    }
  };

  struct PixelHash {
    std::size_t operator()(const Pixel &pixel) const noexcept {
      std::size_t h1 = std::hash<std::size_t>{}(pixel.row);
      std::size_t h2 = std::hash<std::size_t>{}(pixel.col);
      return h1 ^ (h2 << 1); // copied from std::hash documentation...
    }
  };

  Framebuffer(int width, int height, int segments)
      : data_(width * height * 3, 0), mutexes_(segments), width_(width) {
    std::cout << "  ctor\n";
    std::vector<std::size_t> rows(height);
    std::vector<std::size_t> cols(width);
    std::iota(rows.begin(), rows.end(), 0);
    std::iota(cols.begin(), cols.end(), 0);
    std::shuffle(rows.begin(), rows.end(),
                 std::mt19937{std::random_device{}()});
    std::shuffle(cols.begin(), cols.end(),
                 std::mt19937{std::random_device{}()});
    std::cout << "  shuffled\n";

    std::cout << "  reserving map space\n";
    std::cout << "  loading index_map_...\n";
    for (std::size_t col : cols) {
      for (std::size_t row : rows) {
        int segment = (row * width + col) % segments;
        Pixel pixel{row, col, segment};
        index_map_[segment].push_back(pixel);
      }
    }

    std::cout << "  ...done! loading index_map_...\n";
  }

  // Returns the list of indices associted with the given chunk.
  //
  // No bounds-checking is done - if chunk >= nChunks, this will crash at
  // runtime
  //
  // TODO: maybe do some bounds checking or design this better
  const std::vector<Pixel> &chunk_pixels(int chunk) const {
    return index_map_.at(chunk);
  }

  // Write the given pixel. This will lock until it is safe to write whatever
  // segment the pixel happens to be in.
  //
  // WARNING: no bounds checking is done - so be careful
  // TODO: maybe do some bounds checking
  void write_pixel(const Pixel &pixel, const rtiaw::Color &color) {
    std::scoped_lock<std::mutex> lk(mutexes_[pixel.segment]);
    std::size_t start = 3 * (pixel.row * width_ + pixel.col);
    data_[start] = std::clamp(color.red, 0, 255);
    data_[start + 1] = std::clamp(color.green, 0, 255);
    data_[start + 2] = std::clamp(color.blue, 0, 255);
  }

private:
  std::vector<char> data_;
  std::vector<std::mutex> mutexes_;
  int width_;
  // key = chunk, value = pixels assigned to chunk
  std::unordered_map<int, std::vector<Pixel>> index_map_;
};

class Runner {
public:
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
  Runner(int height, int width, double aspect_ratio, unsigned int nThread,
         const std::vector<std::reference_wrapper<rtiaw::Object>> &objects,
         double viewport_height = 2, double focal_length = 1)
      : width_(width), height_(height), objects_(objects),
        camera_(viewport_height, viewport_height * aspect_ratio, focal_length),
        buffer_(width, height, nThread) {}

  void run(int chunk) {
    static const int samples_per_pixel = 100;
    static std::uniform_real_distribution<double> distribution(-0.5, 0.5);
    static std::mt19937 generator;

    for (const Framebuffer::Pixel &pixel : buffer_.chunk_pixels(chunk)) {
      bool hit = false;
      rtiaw::Color color{0, 0, 0};
      for (int sample = 0; sample < samples_per_pixel; ++sample) {
        double u = (pixel.col + distribution(generator)) / double(width_ - 1);
        double v = (pixel.row + distribution(generator)) / double(height_);
        rtiaw::Ray ray = camera_.ray_at(u, v);

        if (sample == 0) {
          color = ray_color(ray);
        }

        double max_t = std::numeric_limits<double>::infinity();
        for (const rtiaw::Object &obj : objects_) {
          if (std::optional<rtiaw::Object::HitData> hd =
                  obj.check_hit(ray, 0, max_t);
              hd.has_value()) {
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
        color.red /= samples_per_pixel;
        color.green /= samples_per_pixel;
        color.blue /= samples_per_pixel;
      }

      buffer_.write_pixel(pixel, color);
    }
  }

private:
  int width_;
  int height_;
  const std::vector<std::reference_wrapper<rtiaw::Object>> &objects_;
  rtiaw::Camera camera_;
  Framebuffer buffer_;
};

int main() {
  std::cout << "hello main\n";
  // image
  const double aspect_ratio = 16.0 / 9.0;
  const int height = 711;
  // const int height = 4;
  /* double aspect_ratio = 1.0; */
  /* int height = 2; */
  const int width = height * aspect_ratio;

  rtiaw::PpmWriter writer("output.ppm", width, height);
  std::ofstream logfile("log.txt", std::ios::out);
  rtiaw::Logger logger;

  // Objects in the world
  std::vector<std::reference_wrapper<rtiaw::Object>> objects;
  rtiaw::Sphere sphere1{rtiaw::Vector{0.0, 0.0, 1.0}, 0.5};
  rtiaw::Sphere sphere2{rtiaw::Vector{0.0, 100.6, 1.0}, 100};
  objects.push_back(sphere1);
  objects.push_back(sphere2);

  std::vector<std::future<void>> futures;
  unsigned int nThread = std::thread::hardware_concurrency() == 0
                             ? 1
                             : std::thread::hardware_concurrency();
  Runner runner(width, height, aspect_ratio, nThread, std::cref(objects));

  // Create the main window
  sf::RenderWindow window(sf::VideoMode(width, height), "raytracer");
  window.setFramerateLimit(60);
  int col = 0;
  int row = 0;
  sf::Clock clock;
  while (window.isOpen()) {
    // Process events
    sf::Event event;
    while (window.pollEvent(event)) {
      // Close window: exit
      if (event.type == sf::Event::Closed)
        window.close();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
      window.close();
    }

    if (clock.getElapsedTime().asMilliseconds() > 5) {
      clock.restart();

      // Clear screen
      window.clear();

      if (col >= width) {
        col = 0;
      }
      if (row >= height) {
        row = 0;
      }

      sf::RectangleShape pixel({1, 1});
      for (int row_adder = 0; row_adder <= 100; ++row_adder) {
        pixel.setPosition(col, row + row_adder);
        window.draw(pixel);
      }

      ++col;
      ++row;
    }

    // Update the window
    window.display();
  }

  // for (unsigned int n = 0; n < nThread; ++n) {
  //   std::cout << "spinning up thread " << n << '\n';
  //   futures.emplace_back(std::async(std::launch::async, &Runner::run,
  //                                   std::reference_wrapper(runner), n));
  // }

  // for (const std::future<void> &future : futures) {
  //   std::cout << "waiting for a future..." << std::endl;
  //   future.wait();
  // }

  // TODO: re-enable some sort of logging
  // i and j represent one pixel each along the +x and +y axes, respectively.
  // for (int j = 0; j < height; ++j) {
  //   for (int i = 0; i < width; ++i) {
  //     std::stringstream sstream;
  //     sstream << (float)j / height * 100 << "% complete";
  //     logger.push(sstream.str());
  //     logger.print_log();
  //   }
  // }
}
