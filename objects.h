#ifndef RTIAW_OBJECTS_HEADER
#define RTIAW_OBJECTS_HEADER

#include <ostream>

#include "vector.h"

namespace rtiaw{
struct Sphere {
  rtiaw::Vector center;
  double radius;
};

struct Ray {
  Vector origin;
  Vector direction;

  Vector at(double t) {
    return origin + t * direction;
  }
};

std::ostream& operator<<(std::ostream& out, const Ray& ray) {
  out << "Ray{origin: " << ray.origin << ", "
      <<     "direction:" << ray.direction << "}";
  return out;
}
} // namespace rtiaw

#endif // RTIAW_OBJECTS_HEADER
