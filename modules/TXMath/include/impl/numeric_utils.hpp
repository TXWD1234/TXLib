// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXMath

#pragma once
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include <type_traits>
#include <concepts>
#include <algorithm>

namespace tx {

// constants

// clang-format off

constexpr float PI         = 3.1415926f; // PI
constexpr float TWO_PI     = 6.2831853f; // PI * 2
constexpr float INV_TWO_PI = 0.1591549f; // 1 / PI * 2
constexpr float HALF_PI    = 1.5707963f; // PI / 2
constexpr float ONE_DEGREE = 0.0174532f; // PI / 180
constexpr float ONE_OF_255 = 0.0039215f; // 1 / 255
constexpr float EPSILON    = 1e-6f; // 1 / 255
// clang-format on

template <class T>
    requires std::is_signed_v<T>
constexpr inline T sign(T val) { return (val > T{ 0 }) - (val < T{ 0 }); }

template <tx::numeric T>
constexpr inline T sq(T in) { return in * in; }

template <class T, T first, T... vals>
    requires std::totally_ordered<T>
inline constexpr T max() {
	T maxVal = first;
	((maxVal = std::max(maxVal, vals)),
	 ...);
	return maxVal;
}
template <class T, T first, T... vals>
    requires std::totally_ordered<T>
inline constexpr T min() {
	T minVal = first;
	((minVal = std::min(minVal, vals)),
	 ...);
	return minVal;
}

template <class T>
constexpr inline bool inRange(T val, T min, T max) { // inclusive
	return val >= min && val <= max;
}

constexpr inline int makeOdd(int in) { // by ++
	return (in % 2 ? in : in + 1);
}

constexpr inline bool isInt(float f) {
	return std::fabs(f - std::round(f)) < EPSILON;
}
} // namespace tx