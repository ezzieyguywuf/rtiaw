#include <chrono>
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

int main () {
  // TODO: get rid of this
  using namespace std::chrono_literals;
  const int width = 5;
  const int height = 5;
  const std::size_t log_rows = 4;

  std::vector<std::string> log_data;
  std::queue<std::string, std::vector<std::string>> log_queue(log_data);
  Logger logger(log_rows);

  std::ofstream outfile("test.ppm", std::ios::out);
  init_ppm(outfile, width, height);

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      Color color;

      color.red = i;
      color.green = CMAX - j;
      color.blue = 255 / 4;

      write_pixel(outfile, color);

      std::string data = "Row: " + std::to_string(j) + ", Col: " + std::to_string(i);
      push(log_data, data, log_rows);
      logger.log(log_data);
      std::this_thread::sleep_for(200ms);
    }
  }
}
