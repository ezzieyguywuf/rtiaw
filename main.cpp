#include <fstream>
#include <iostream>
#include <queue>
#include <string>
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

// Logs the data in the fifo to std::cout, using ANSI escape sequences to reuse
// the same area of the terminal rather than scrolling
void log(const std::vector<std::string>& data) {
  std::cout << "logging...\n";
  for (const std::string& d : data) {
    std::cout << d << '\n';
  }
  std::cout << "...logged\n";
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

int main () {
  const int width = 5;
  const int height = 5;

  std::vector<std::string> log_data;
  std::queue<std::string, std::vector<std::string>> log_queue(log_data);


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
      push(log_data, data, 4);
      log(log_data);
    }
  }
}
