#ifndef RTIAW_VECTOR_HEADER
#define RTIAW_VECTOR_HEADER

#include <cmath>
#include <ostream>

namespace rtiaw {
struct Vector {
  double dx;
  double dy;
  double dz;

  Vector operator*(double val) const { return {dx * val, dy * val, dz * val}; }

  Vector operator/(double val) const { return {dx / val, dy / val, dz / val}; }

  Vector operator+(const Vector &other) const {
    return {dx + other.dx, dy + other.dy, dz + other.dz};
  }

  Vector operator-(const Vector &other) const {
    return {dx - other.dx, dy - other.dy, dz - other.dz};
  }
};

std::ostream &operator<<(std::ostream &out, const Vector &vec) {
  out << "{" << vec.dx << ", " << vec.dy << ", " << vec.dz << "}";
  return out;
}

Vector operator*(double val, const Vector &vec) { return vec * val; }

Vector operator/(double val, const Vector &vec) { return vec / val; }

double vector_length(const Vector &vec) {
  return std::sqrt(vec.dx * vec.dx + vec.dy * vec.dy + vec.dz * vec.dz);
}

double dot(const Vector &u, const Vector &v) {
  return u.dx * v.dx + u.dy * v.dy + u.dz * v.dz;
}

Vector unit_vector(const Vector &vec) { return vec / vector_length(vec); }

} // namespace rtiaw

#endif // RITAW_VECTOR_HEADER
