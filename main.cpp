#include <fstream>

int main () {
  std::ofstream outfile("test.ppm", std::ios::out);
  outfile << "Hello, Ray Tracing in a weekend!\n";
}
