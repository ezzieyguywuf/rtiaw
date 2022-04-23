#ifndef RTIAW_OBJECTS_HEADER
#define RTIAW_OBJECTS_HEADER

#include <cfloat>
#include <ostream>
#include <optional>

#include "vector.h"

namespace rtiaw{
struct Ray {
  Vector origin;
  Vector direction;

  Vector at(double t) {
    return origin + t * direction;
  }
};

// An object in 3D space. This object can be hit by a ray
class Object {
 public:
   virtual std::optional<double> check_hit(const Ray& ray) const {
     return check_hit(ray, 0, DBL_MAX);
   }

  // If the ray hits the object, returns the t paramater at wherh the ray hits
  // the object. In other words, ray.at(t) is where the hit occurs
  virtual std::optional<double> check_hit(
      const Ray& ray, double t_min, double t_max) const = 0;
};

struct Sphere : public Object {
  Vector center;
  double radius;

  Sphere(const Vector& center, double radius) : center(center), radius(radius) {};

  // General equation for a sphere: (x - Cx)² + (y - Cy)² + (z - Cz)² = r²
  // in vector form, C = {Cx, Cy, Cz}, P = {x, y, z}, so (P - C) · (P - C) = r²
  //
  // This can be solved using the quadratic formula.
  //
  // We need to expand P to P = A + t·B, which is our equation for our ray.
  //   A = origin of ray
  //   B = direction of ray
  //
  // (P - C) · (P - C) = (A + t·B - C) · (A + t·B - C)
  //                   = (B·B)t² - 2(B·(A - C))t + ((A-C)·(A-C) - r²) = 0
  std::optional<double> check_hit(const Ray& ray, double t_min, double t_max) const override {
    // (A - C) in equations above
    Vector ca = ray.origin - center;
    double a = dot(ray.direction, ray.direction);
    double b = 2 * dot(ca, ray.direction);
    double c = dot(ca, ca) - (radius * radius);

    double discriminant = b*b - 4*a*c;
    if (discriminant < 0) {
      return std::nullopt;
    }

    // Find the nearest root that lies in the acceptable range
    double sqrtd = std::sqrt(discriminant);
    double root1 = -b - sqrtd / (2.0 * a);
    double root2 = -b + sqrtd / (2.0 * a);

    // there's a few different scenareos here - either or both of the roots
    // could be outside the t_min t_max range. In case both are within range, we
    // want to return the smallest of the two
    bool root1_in_range = (root1 >= t_min) && (root1 <= t_max);
    bool root2_in_range = (root2 >= t_min) && (root2 <= t_max);
    if ( root1_in_range && root2_in_range ) {
      return root1 < root2 ? root1 : root2;
    } else if (root1_in_range) {
      return root1;
    } else if (root2_in_range) {
      return root2;
    } else {
      return std::nullopt;
    }
  }
};


std::ostream& operator<<(std::ostream& out, const Ray& ray) {
  out << "Ray{origin: " << ray.origin << ", "
      <<     "direction:" << ray.direction << "}";
  return out;
}
} // namespace rtiaw

#endif // RTIAW_OBJECTS_HEADER
