// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXMath

#pragma once
#include <cmath>
#include <iostream>

namespace tx {

constexpr float epsilon = 1e-6f;

// vec2 ***********************************************************************************************************************

// 2 direction vector
class vec2;
class Coord;
constexpr inline float dot(const vec2& in1, const vec2& in2);
class vec2 {
public:
	float x, y;

	constexpr vec2(float in_x, float in_y) : x(in_x), y(in_y) {}
	constexpr vec2() : x(0.0f), y(0.0f) {}

	constexpr inline void set(float in_x, float in_y) {
		this->x = in_x;
		this->y = in_y;
	}

	constexpr inline vec2 operator+(const vec2& other) const { return vec2(this->x + other.x, this->y + other.y); }
	constexpr inline vec2 operator+(float other) const { return vec2(this->x + other, this->y + other); }
	constexpr inline vec2 operator-(const vec2& other) const { return vec2(this->x - other.x, this->y - other.y); }
	constexpr inline vec2 operator-(float other) const { return vec2(this->x - other, this->y - other); }
	constexpr inline vec2 operator*(float coef) const { return vec2(this->x * coef, this->y * coef); }
	constexpr inline vec2 operator/(float coef) const { return vec2(this->x / coef, this->y / coef); }
	constexpr inline vec2& operator+=(const vec2& other) {
		this->x += other.x;
		this->y += other.y;
		return *this;
	}
	constexpr inline vec2& operator+=(float other) {
		this->x += other;
		this->y += other;
		return *this;
	}
	constexpr inline vec2& operator-=(const vec2& other) {
		this->x -= other.x;
		this->y -= other.y;
		return *this;
	}
	constexpr inline vec2& operator-=(float other) {
		this->x -= other;
		this->y -= other;
		return *this;
	}
	constexpr inline vec2& operator*=(float coef) {
		this->x *= coef;
		this->y *= coef;
		return *this;
	}
	inline bool operator==(const vec2& other) const { return std::fabs(this->x - other.x) <= epsilon && std::fabs(this->y - other.y) <= epsilon; }
	inline bool operator!=(const vec2& other) const { return !(this->operator==(other)); }
	constexpr inline bool operator<(const vec2& other) const { return this->x * this->x + this->y * this->y < other.x * other.x + other.y * other.y; }
	constexpr inline bool operator>(const vec2& other) const { return this->x * this->x + this->y * this->y > other.x * other.x + other.y * other.y; }

	inline float length() const {
		return std::sqrtf(dot(*this, *this));
	}

	constexpr inline vec2 offset(float x, float y) const { return this->operator+(vec2(x, y)); }
	constexpr inline vec2 offsetX(float in) const { return vec2(this->x + in, this->y); }
	constexpr inline vec2 offsetY(float in) const { return vec2(this->x, this->y + in); }

	constexpr inline void moveX(float in) { this->x += in; }
	constexpr inline void moveY(float in) { this->y += in; }
};
constexpr inline vec2 operator*(float coef, vec2 vec) { return vec2(vec.x * coef, vec.y * coef); }
constexpr inline vec2 operator/(float coef, vec2 vec) { return vec2(vec.x / coef, vec.y / coef); }

// Coord **********************************************************************************************************************

// 2d integer coordinate
class Coord {
public:
	int x, y;

	constexpr Coord(int in_x, int in_y) : x(in_x), y(in_y) {}
	constexpr Coord(int in_pos) : x(in_pos), y(in_pos) {}
	constexpr Coord() : x(0), y(0) {}
	constexpr inline void set(int in_x, int in_y) {
		this->x = in_x;
		this->y = in_y;
	}
	constexpr Coord(const Coord&) = default;

	constexpr inline Coord operator+(const Coord& other) const { return Coord(this->x + other.x, this->y + other.y); }
	constexpr inline Coord operator-(const Coord& other) const { return Coord(this->x - other.x, this->y - other.y); }
	constexpr inline Coord operator+=(const Coord& other) {
		this->x += other.x;
		this->y += other.y;
		return *this;
	}
	constexpr inline Coord operator-=(const Coord& other) {
		this->x -= other.x;
		this->y -= other.y;
		return *this;
	}

	constexpr inline Coord operator=(const Coord& other) {
		this->x = other.x;
		this->y = other.y;
		return *this;
	}
	constexpr inline bool operator==(const Coord& other) const { return this->x == other.x && this->y == other.y; }
	constexpr inline bool operator!=(const Coord& other) const { return !(this->x == other.x && this->y == other.y); }
	constexpr inline bool operator<(const Coord& other) const { return this->x * this->x + this->y * this->y < other.x * other.x + other.y * other.y; }
	constexpr inline bool operator>(const Coord& other) const { return this->x * this->x + this->y * this->y > other.x * other.x + other.y * other.y; }

	constexpr inline vec2 operator*(float in) const { return vec2{ x * in, y * in }; }

	constexpr inline bool valid(int edge) {
		return x >= 0 && y >= 0 && x < edge && y < edge;
	}

	constexpr inline Coord offset(int in_x, int in_y) const { return Coord{ this->x + in_x, this->y + in_y }; }
	constexpr inline Coord offsetX(int in) const { return Coord(this->x + in, this->y); }
	constexpr inline Coord offsetY(int in) const { return Coord(this->x, this->y + in); }
	constexpr inline void move(int in_x, int in_y) {
		this->x += in_x;
		this->y += in_y;
	}
	constexpr inline void moveX(int in) { this->x += in; }
	constexpr inline void moveY(int in) { this->y += in; }
};

inline Coord toCoord(const vec2& in) { return Coord{ static_cast<int>(std::floor(in.x)), static_cast<int>(std::floor(in.y)) }; }
inline vec2 toVec2(const Coord& in) { return vec2{ static_cast<float>(in.x), static_cast<float>(in.y) }; }

inline std::ostream& operator<<(std::ostream& in_cout, const vec2& in_vec2) {
	in_cout << "( " << in_vec2.x << ", " << in_vec2.y << " )";
	return in_cout;
}
inline std::ostream& operator<<(std::ostream& in_cout, const Coord& in_coord) {
	in_cout << "( " << in_coord.x << ", " << in_coord.y << " )";
	return in_cout;
}

// constants **************************************************************************************************************
// clang-format off

constexpr const vec2 IHat(1.0f, 0.0f);
constexpr const vec2 JHat(0.0f, 1.0f);
constexpr const vec2 Origin(0.0f, 0.0f);
constexpr const vec2 InvalidVec(NAN, NAN);

constexpr vec2 TopRight = { 1.0, 1.0 };
constexpr vec2 TopLeft = { -1.0, 1.0 };
constexpr vec2 BottomLeft = { -1.0, -1.0 };
constexpr vec2 BottomRight = { 1.0, -1.0 };

constexpr vec2 directionVec[] = {
    {  1.0f,  0.0f },
    {  1.0f,  1.0f },
    {  0.0f,  1.0f },
    { -1.0f,  1.0f },
    { -1.0f,  0.0f },
    { -1.0f, -1.0f },
    {  0.0f, -1.0f },
    {  1.0f, -1.0f }
};

constexpr Coord _8wayIncrement[] = {
    {  1,  0 },
    {  1,  1 },
    {  0,  1 },
    { -1,  1 },
    { -1,  0 },
    { -1, -1 },
    {  0, -1 },
    {  1, -1 }
};
constexpr Coord _4wayIncrement[] = {
	{  1,  0 },
	{ -1,  0 },
	{  0,  1 },
	{  0, -1 }
};
constexpr int _2wayIncrement[] = {
	1, -1
};
constexpr Coord CoordOrigin{ 0, 0 };
// clang-format on













} // namespace tx

#include <functional>
// std::hash specialization so tx::Coord can be used in std::unordered_map
namespace std {
template <>
struct hash<tx::Coord> {
	size_t operator()(const tx::Coord& c) const noexcept {
		size_t h1 = std::hash<int>{}(c.x);
		size_t h2 = std::hash<int>{}(c.y);
		// Standard hash combining algorithm (from Boost)
		return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
	}
};
} // namespace std