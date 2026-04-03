// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXMath
// File: basic_utils.h

#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include <concepts>

namespace tx {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using f32 = float;
using f64 = double;

// constants

constexpr float PI = 3.1415926f;
constexpr float TWO_PI = 6.283185307f;
constexpr float INV_TWO_PI = 0.1591549431;
constexpr float HALF_PI = 1.570796327;
constexpr float ONE_DEGREE = 0.017453292f;
constexpr float oneOf255 = 1.0f / 255.0f;

constexpr u64 InvalidU64 = UINT64_MAX;
constexpr u32 InvalidU32 = UINT32_MAX;
constexpr u16 InvalidU16 = UINT16_MAX;
constexpr u8 InvalidU8 = UINT8_MAX;

template <class T>
constexpr inline T sign(T num) { return (num == 0 ? 1 : num / std::abs(num)); }
constexpr inline float sq(float in) { return in * in; }
constexpr inline double sq(double in) { return in * in; }
constexpr inline int sq(int in) { return in * in; }
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
template <class T>
constexpr inline T min(T a, T b) { return a < b ? a : b; }
template <class T>
constexpr inline T max(T a, T b) { return a > b ? a : b; }
template <class T>
constexpr inline T min(const std::vector<T>& vec) {
	if (!vec.size()) {
		return T{};
	}
	T minv = vec[0];
	for (const T& i : vec) {
		minv = min(minv, i); // 1
		// if (i < minv) minv = i; // 2
	}
	return minv;
}
template <class T>
constexpr inline T max(const std::vector<T>& vec) {
	if (!vec.size()) {
		return T{};
	}
	T maxv = vec[0];
	for (const T& i : vec) {
		maxv = max(maxv, i);
	}
	return maxv;
}
template <class T>
constexpr inline T clamp(const T& val, const T& in_min, const T& in_max) {
	return max(min(val, in_max), in_min);
}
template <class T>
inline T sum(const std::vector<T>& vec) {
	if (!vec.size())
		return T{};
	T sum{};
	for (int i = 0; i < vec.size(); i++) {
		sum += vec[i];
	}
	return sum;
}
template <class T>
inline double average(const std::vector<T>& vec) {
	if (!vec.size())
		return T{};
	return sum(vec) / (double)vec.size();
}
template <class T>
constexpr inline bool inRange(T val, T min, T max) { // inclusive
	return val >= min && val <= max;
}
constexpr inline int makeOdd(int in) { // by ++
	return (in % 2 ? in : in + 1);
}
constexpr inline bool isInt(float f) {
	return std::fabs(f - std::round(f)) < 1e-6f;
}
constexpr inline bool valid(u64 in) { return in != InvalidU64; }
constexpr inline bool valid(u32 in) { return in != InvalidU32; }
constexpr inline bool valid(u16 in) { return in != InvalidU16; }
constexpr inline bool valid(u8 in) { return in != InvalidU8; }

// algorithms

// don't use this, it's bad
constexpr inline float fast_sin_noRangeReduction(float x) {
	// Mirror to [-PI/2, PI/2] to keep Taylor series accurate: using identity: sin(x) == sin(PI - x)
	if (x > HALF_PI)
		x = PI - x;
	else if (x < -HALF_PI)
		x = -PI - x;

	float x2 = sq(x);

	// Taylor's Series: x - x^3/3! + x^5/5!
	return x * (1.0f - x2 * (0.166666667f - x2 * 0.008333333f));
}
// don't use this, it's bad
constexpr inline float fast_sin(float x) {
	// Range reduction to [-PI, PI]
	x = x - TWO_PI * std::floor((x + PI) * INV_TWO_PI);
	return fast_sin_noRangeReduction(x);
}
// don't use this, it's bad
constexpr inline float fast_cos(float x) {
	return fast_sin(x + HALF_PI);
}


} // namespace tx