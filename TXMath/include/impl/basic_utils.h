// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXMath
// File: basic_utils.h

#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

namespace tx {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using f32 = float;
using f64 = double;

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
} // namespace tx