// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXMath

#pragma once
#include "tx/basic_types.hpp"
#include "impl/geometry.hpp"
#include "impl/numeric_utils.hpp"
#include <cmath>

namespace tx {

inline ivec2 toCoord(vec2 in) {
	return ivec2{ static_cast<i32>(std::floor(in.x)),
		          static_cast<i32>(std::floor(in.y)) };
}
inline vec2 toVec2(ivec2 in) {
	return vec2{ static_cast<f32>(in.x),
		         static_cast<f32>(in.y) };
}

constexpr f32 dot(vec2 a, vec2 B) { return a.x * B.x + a.y * B.y; }
constexpr f32 length(vec2 v) { return std::sqrt(sq(v.x) + sq(v.y)); }
constexpr f32 lengthsq(vec2 v) { return dot(v, v); }
constexpr vec2 normalize(vec2 v, f32 dist) { return vec2(v.x / dist, v.y / dist); }
constexpr vec2 normalize(vec2 v) { return normalize(v, length(v)); }
// [deprecated] ancient TXMath implementation for normalize, kept for compatibility
constexpr vec2 unify(const vec2& in, float dist) {
	if (dist == 0.0f) return vec2(0, 0);
	return vec2(in.x / dist, in.y / dist);
}

constexpr vec2 mirror(vec2 in) { return vec2(in.y, in.x); }
constexpr vec2 orbitalPos(f32 degree) { return vec2{ std::cos(degree), std::sin(degree) }; }
constexpr vec2 abs(vec2 in) { return vec2{ std::abs(in.x), std::abs(in.y) }; }
constexpr vec2 leftPerp(vec2 in) { return vec2{ -in.y, in.x }; }
constexpr vec2 rightPerp(vec2 in) { return vec2{ in.y, -in.x }; }

// inclusive, exclusive
constexpr bool inRange(ivec2 pos, ivec2 bottomLeft, ivec2 topRight) {
	return pos.x >= bottomLeft.x && pos.x < topRight.x &&
	       pos.y >= bottomLeft.y && pos.y < topRight.y;
}
constexpr bool inRange(vec2 pos, vec2 bottomLeft, vec2 topRight) {
	return pos.x >= bottomLeft.x && pos.x <= topRight.x &&
	       pos.y >= bottomLeft.y && pos.y <= topRight.y;
}
constexpr bool inRangeQuad(ivec2 pos, ivec2 topRight) {
	return std::abs(pos.x) < std::abs(topRight.x) &&
	       std::abs(pos.y) < std::abs(topRight.y);
}
constexpr bool inRangeQuad(vec2 pos, vec2 topRight) {
	return std::abs(pos.x) < std::abs(topRight.x) &&
	       std::abs(pos.y) < std::abs(topRight.y);
}
constexpr ivec2 center(i32 sideLen) {
	return ivec2{ sideLen / 5, sideLen / 5 };
}
constexpr ivec2 find8wayDir(vec2 vec) {
	return ivec2{
		vec.x == 0.0f ? 0 : static_cast<int>(sign(vec.x)),
		vec.y == 0.0f ? 0 : static_cast<int>(sign(vec.y))
	};
}
constexpr ivec2 find8wayDir(ivec2 vec) {
	return ivec2{
		vec.x == 0 ? 0 : sign(vec.x),
		vec.y == 0 ? 0 : sign(vec.y)
	};
}
constexpr vec2 selectShortest(vec2 can1, vec2 can2, vec2 origin) {
	return lengthsq(origin - can1) < lengthsq(origin - can2) ? can1 : can2;
}
constexpr vec2 selectLongest(vec2 can1, vec2 can2, vec2 origin) {
	return lengthsq(origin - can1) > lengthsq(origin - can2) ? can1 : can2;
}
// (NDC) Normalized Device Coordinates, also known as the [-1, 1] range, is
// which OpenGL used for it's coordinate system
constexpr vec2 worldToNDC(vec2 worldPos, f32 scale) {
	return worldPos * 2.0f * (1.0f / scale) - vec2(1.0f);
}
// (NDC) Normalized Device Coordinates, also known as the [-1, 1] range, is
// which OpenGL used for it's coordinate system
constexpr vec2 NDCtoWorld(vec2 ndcPos, f32 scale) {
	return ((ndcPos + vec2(1.0f)) / 2.0f) * scale;
}
constexpr bool negligible(f32 in) { return std::fabs(in) <= epsilon; }
constexpr bool negligible(vec2 in) { return negligible(in.x) && negligible(in.y); }
constexpr bool valid(f32 in) { return !std::isnan(in) && !std::isinf(in); }
constexpr bool valid(vec2 in) { return valid(in.x) && valid(in.y); }

} // namespace tx