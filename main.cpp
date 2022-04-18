#include <fstream>

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

int main () {
  const int width = 256;
  const int height = 256;

  std::ofstream outfile("test.ppm", std::ios::out);
  init_ppm(outfile, width, height);

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      Color color;

      color.red = i;
      color.green = CMAX - j;
      color.blue = 255 / 4;

      write_pixel(outfile, color);
    }
  }
}
