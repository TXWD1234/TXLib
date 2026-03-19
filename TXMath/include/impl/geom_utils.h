// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXMath
// File: geom_utils.h

#pragma once
#include "impl/basic_utils.h"
#include "impl/geometry.h"
#include <cmath>

namespace tx {

inline float dot(const vec2& in1, const vec2& in2) { return (in1.getX() * in2.getX() + in1.getY() * in2.getY()); }
inline float hypotsq(const vec2& in) { return dot(in, in); }
inline vec2 unify(const vec2& in, float* len = nullptr) {
	float scalar = std::sqrtf(dot(in, in));
	if (scalar == 0.0f)
		return vec2(0, 0);
	if (len != nullptr)
		*len = scalar;
	return vec2(in.getX() / scalar, in.getY() / scalar);
}
constexpr inline vec2 unify(const vec2& in, float dist) {
	if (dist == 0.0f)
		return vec2(0, 0);
	return vec2(in.getX() / dist, in.getY() / dist);
}
constexpr inline vec2 reverse(const vec2& in) { return in * -1.0f; }
constexpr inline vec2 operator-(const vec2& in) { return in * -1.0f; }
constexpr inline vec2 mirror(const vec2& in) { return vec2(in.y(), in.x()); }

inline vec2 orbitalPos(float degree) { return vec2{ std::cosf(degree), std::sinf(degree) }; }
inline vec2 abs(const vec2& in) { return vec2{ std::fabs(in.x()), std::fabs(in.y()) }; }
inline bool isValid(double in) { return !std::isnan(in) && !std::isinf(in); }
inline bool isValid(const vec2& in) { return isValid(in.x()) && isValid(in.y()); }
constexpr inline vec2 leftPerp(const vec2& in) { return vec2{ -in.y(), in.x() }; }
constexpr inline vec2 rightPerp(const vec2& in) { return vec2{ in.y(), -in.x() }; }

// inclusive, exclusive
constexpr inline bool inRange(const Coord& pos, const Coord& bottomLeft, const Coord& topRight) { // inclusive, exclusive
	return pos.x() >= bottomLeft.x() && pos.x() < topRight.x() && pos.y() >= bottomLeft.y() && pos.y() < topRight.y();
}
constexpr inline bool inRange(const vec2& pos, const vec2& bottomLeft, const vec2& topRight) {
	return pos.x() >= bottomLeft.x() && pos.x() <= topRight.x() && pos.y() >= bottomLeft.y() && pos.y() <= topRight.y();
}
constexpr inline bool inRange_p(const Coord& pos, const Coord& topRight) {
	return (unsigned)pos.x() < (unsigned)topRight.x() && (unsigned)pos.y() < (unsigned)topRight.y();
}
constexpr inline bool inRange_p(const vec2& pos, const vec2& topRight) {
	return (unsigned)pos.x() < (unsigned)topRight.x() && (unsigned)pos.y() < (unsigned)topRight.y();
}
constexpr inline Coord center(int sideLen) {
	return Coord{ static_cast<int>(sideLen * 0.5) };
}
inline Coord find8wayDir(const vec2& vec) {
	return Coord{
		vec.x() == 0.0f ? 0 : static_cast<int>(sign(vec.x())),
		vec.y() == 0.0f ? 0 : static_cast<int>(sign(vec.y()))
	};
}
inline Coord find8wayDir(const Coord& vec) {
	return Coord{
		vec.x() == 0 ? 0 : sign(vec.x()),
		vec.y() == 0 ? 0 : sign(vec.y())
	};
}
inline vec2 selectShortest(const vec2& can1, const vec2& can2, const vec2& origin) {
	return (origin - can1) < (origin - can2) ? can1 : can2;
}
inline vec2 selectLongest(const vec2& can1, const vec2& can2, const vec2& origin) {
	return (origin - can1) > (origin - can2) ? can1 : can2;
}
// (NDC) Normalized Device Coordinates, also known as the [-1, 1] range, is which OpenGL used for it's coordinate system
inline vec2 worldToNDC(const vec2& worldPos, float scale) {
	return worldPos * 2.0f / scale - 1.0f;
}
// (NDC) Normalized Device Coordinates, also known as the [-1, 1] range, is which OpenGL used for it's coordinate system
inline vec2 NDCtoWorld(const vec2& pos, float scale) {
	return (pos + 1.0f) / 2.0f * scale;
}

} // namespace tx
