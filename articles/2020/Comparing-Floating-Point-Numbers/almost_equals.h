// https://bot-man-jl.github.io/articles/?post=2020/Comparing-Floating-Point-Numbers

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <limits>
#include <type_traits>

template <class Flp>
bool Equals(Flp f1, Flp f2) {
  return f1 == f2;
}

template <class Flp>
bool AlmostEqualsAbs(Flp f1,
                     Flp f2,
                     Flp epsilon = std::numeric_limits<Flp>::epsilon()) {
  return fabs(f1 - f2) <= epsilon;
}

template <class Flp>
bool AlmostEqualsRel(Flp f1,
                     Flp f2,
                     Flp epsilon = std::numeric_limits<Flp>::epsilon()) {
  return fabs(f1 - f2) <= epsilon * std::max(fabs(f1), fabs(f2));
}

namespace detail {

// Reference:
// https://github.com/google/googletest/blob/master/googletest/include/gtest/internal/gtest-internal.h

template <typename Flp>
class FloatingPoint {
 public:
  FloatingPoint(const Flp& x, size_t max_ulps = 4) : max_ulps_(max_ulps) {
    u_.value_ = x;
  }

  // - returns false if either number is (or both are) NAN.
  // - treats really large numbers as almost equal to infinity.
  // - thinks +0.0 and -0.0 are 0 DLP's apart.
  bool AlmostEquals(const FloatingPoint& rhs) const {
    if (is_nan() || rhs.is_nan())
      return false;

    const Bits biased1 = SignAndMagnitudeToBiased();
    const Bits biased2 = rhs.SignAndMagnitudeToBiased();
    return (std::max(biased1, biased2) - std::min(biased1, biased2)) <=
           max_ulps_;
  }

 private:
  using Bits = std::conditional_t<
      sizeof(Flp) == 4,
      uint32_t,
      std::conditional_t<sizeof(Flp) == 8, uint64_t, uint8_t>>;
  static_assert(sizeof(Bits) == 4 || sizeof(Bits) == 8, "only float or double");

  static constexpr size_t kBitCount = 8 * sizeof(Flp);
  static constexpr size_t kFractionBitCount =
      std::numeric_limits<Flp>::digits - 1;
  static constexpr size_t kExponentBitCount = kBitCount - 1 - kFractionBitCount;

  static constexpr Bits kSignBitMask = static_cast<Bits>(1) << (kBitCount - 1);
  static constexpr Bits kFractionBitMask = ~static_cast<Bits>(0) >>
                                           (kExponentBitCount + 1);
  static constexpr Bits kExponentBitMask = ~(kSignBitMask | kFractionBitMask);

  bool is_nan() const {
    // It's a NAN if the exponent bits are all ones and the fraction
    // bits are not entirely zeros.
    return ((kExponentBitMask & u_.bits_) == kExponentBitMask) &&
           ((kFractionBitMask & u_.bits_) != 0);
  }

  // Converts an integer from the sign-and-magnitude representation to
  // the biased representation.  More precisely, let N be 2 to the
  // power of (kBitCount - 1), an integer x is represented by the
  // unsigned number x + N.
  //
  // For instance,
  //
  //   -N + 1 (the most negative number representable using
  //          sign-and-magnitude) is represented by 1;
  //   0      is represented by N; and
  //   N - 1  (the biggest number representable using
  //          sign-and-magnitude) is represented by 2N - 1.
  //
  // Read http://en.wikipedia.org/wiki/Signed_number_representations
  // for more details on signed number representations.
  Bits SignAndMagnitudeToBiased() const {
    return (kSignBitMask & u_.bits_)
               ? (~u_.bits_ + 1)             // represents a negative number.
               : (kSignBitMask | u_.bits_);  // represents a positive number.
  }

  union FloatingPointUnion {
    Flp value_;
    Bits bits_;
  };

  FloatingPointUnion u_;
  size_t max_ulps_ = 0;
};

}  // namespace detail

template <class Flp>
bool AlmostEqualsUlp(Flp lhs, Flp rhs, size_t max_ulps = 4) {
  using FloatingPoint = detail::FloatingPoint<Flp>;
  return FloatingPoint(lhs, max_ulps).AlmostEquals(FloatingPoint(rhs));
}
