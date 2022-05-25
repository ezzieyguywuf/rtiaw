#include <algorithm>
#include <fstream>
#include <functional>
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

// TODO: make this function signature betterrer
void runner(Framebuffer &writer, int chunk,
            const std::vector<std::reference_wrapper<rtiaw::Object>> &objects,
            const rtiaw::Camera &cam, int width, int height) {
  static const int samples_per_pixel = 100;
  static std::uniform_real_distribution<double> distribution(-0.5, 0.5);
  static std::mt19937 generator;

  for (const Framebuffer::Pixel &pixel : writer.chunk_pixels(chunk)) {
    bool hit = false;
    rtiaw::Color color{0, 0, 0};
    for (int sample = 0; sample < samples_per_pixel; ++sample) {
      double u = (pixel.col + distribution(generator)) / double(width - 1);
      double v = (pixel.row + distribution(generator)) / double(height);
      rtiaw::Ray ray = cam.ray_at(u, v);

      if (sample == 0) {
        color = ray_color(ray);
      }

      double max_t = std::numeric_limits<double>::infinity();
      for (const rtiaw::Object &obj : objects) {
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

    writer.write_pixel(pixel, color);
  }
}

int main() {
  std::cout << "hello main\n";
  // image
  const double aspect_ratio = 16.0 / 9.0;
  const int height = 711;
  // const int height = 4;
  /* double aspect_ratio = 1.0; */
  /* int height = 2; */
  const int width = height * aspect_ratio;

  static const unsigned int nThread = std::thread::hardware_concurrency() == 0
                                          ? 1
                                          : std::thread::hardware_concurrency();
  std::cout << "making buffer\n";
  Framebuffer buffer(width, height, nThread);
  std::cout << "....done making buffer\n";

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

  logfile << "width: " << width << ", height: " << height
          << ", aspect_ratio: " << aspect_ratio << '\n';
  logfile << "viewport_width: " << viewport_width
          << ", viewport_height: " << viewport_height << '\n';

  // TODO: where you left off - fire off one thread per chunk to call runner
  // then one more thread (I guess?) to periodically write the Framebuffer to
  // disk
  std::vector<std::thread> threads;
  for (unsigned int n = 0; n < nThread; ++n) {
    std::cout << "spinning up thread " << n << '\n';
    threads.emplace_back(runner, std::ref(buffer), 0, std::cref(objects),
                         std::cref(cam), width, height);
  }

  for (std::thread &thread : threads) {
    std::cout << "waiting for thread...\n";
    thread.join();
  }

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
