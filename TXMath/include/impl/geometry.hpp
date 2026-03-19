// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXMath
// File: geometry.h

#pragma once
#include <cmath>
#include <iostream>

namespace tx {

constexpr float epsilon = 1e-6f;

// vec2 ***********************************************************************************************************************

// 2 direction vector
class vec2;
class Coord;
inline float dot(const vec2& in1, const vec2& in2);
class vec2 {
public:
	constexpr vec2(float in_x, float in_y) : m_x(in_x), m_y(in_y) {
	}
	constexpr vec2() : m_x(0.0f), m_y(0.0f) {}
	//explicit constexpr vec2(const Coord& in) : m_x(in.x()), m_y(in.y()) {}

	constexpr inline void set(float in_x, float in_y) {
		this->m_x = in_x;
		this->m_y = in_y;
	}
	constexpr inline void setX(float in_x) { this->m_x = in_x; }
	constexpr inline void setY(float in_y) { this->m_y = in_y; }
	constexpr inline float getX() const { return this->m_x; }
	constexpr inline float x() const { return this->m_x; }
	constexpr inline float getY() const { return this->m_y; }
	constexpr inline float y() const { return this->m_y; }

	constexpr inline vec2 operator+(const vec2& other) const { return vec2(this->m_x + other.m_x, this->m_y + other.m_y); }
	constexpr inline vec2 operator+(float other) const { return vec2(this->m_x + other, this->m_y + other); }
	constexpr inline vec2 operator-(const vec2& other) const { return vec2(this->m_x - other.m_x, this->m_y - other.m_y); }
	constexpr inline vec2 operator-(float other) const { return vec2(this->m_x - other, this->m_y - other); }
	//constexpr inline vec2 operator*(const vec2& other) const { return vec2(this->m_x * other.m_x, this->m_y * other.m_y); }
	constexpr inline vec2 operator*(float coef) const { return vec2(this->m_x * coef, this->m_y * coef); }
	constexpr inline vec2 operator/(float coef) const { return vec2(this->m_x / coef, this->m_y / coef); }
	constexpr inline vec2& operator+=(const vec2& other) {
		this->m_x += other.m_x;
		this->m_y += other.m_y;
		return *this;
	}
	constexpr inline vec2& operator+=(float other) {
		this->m_x += other;
		this->m_y += other;
		return *this;
	}
	constexpr inline vec2& operator-=(const vec2& other) {
		this->m_x -= other.m_x;
		this->m_y -= other.m_y;
		return *this;
	}
	constexpr inline vec2& operator-=(float other) {
		this->m_x -= other;
		this->m_y -= other;
		return *this;
	}
	//constexpr inline vec2& operator*=(const vec2& other) { this->m_x *= other.m_x; this->m_y *= other.m_y; return *this; }
	constexpr inline vec2& operator*=(float coef) {
		this->m_x *= coef;
		this->m_y *= coef;
		return *this;
	}
	//vec2 operator/(const vec2& other) { return vec2(this->m_x / other.m_x, this->m_y / other.m_y); }

	//constexpr inline vec2& operator=(const vec2& other) { this->m_x = other.m_x; this->m_y = other.m_y; return *this; }
	//constexpr inline vec2& operator=(float other) { this->m_x = other; this->m_y = other; return *this; }
	inline bool operator==(const vec2& other) const { return std::fabs(this->m_x - other.m_x) <= epsilon && std::fabs(this->m_y - other.m_y) <= epsilon; }
	inline bool operator!=(const vec2& other) const { return !(this->operator==(other)); }
	constexpr inline bool operator<(const vec2& other) const { return this->m_x * this->m_x + this->m_y * this->m_y < other.m_x * other.m_x + other.m_y * other.m_y; }
	constexpr inline bool operator>(const vec2& other) const { return this->m_x * this->m_x + this->m_y * this->m_y > other.m_x * other.m_x + other.m_y * other.m_y; }

	inline float length() const {
		return std::sqrtf(dot(*this, *this));
	}

	constexpr inline vec2 offset(float x, float y) const { return this->operator+(vec2(x, y)); }
	constexpr inline vec2 offsetX(float in) const { return vec2(this->m_x + in, this->m_y); }
	constexpr inline vec2 offsetY(float in) const { return vec2(this->m_x, this->m_y + in); }

private:
	float m_x, m_y;
};
constexpr inline vec2 operator*(float coef, vec2 vec) { return vec2(vec.getX() * coef, vec.getY() * coef); }
constexpr inline vec2 operator/(float coef, vec2 vec) { return vec2(vec.getX() / coef, vec.getY() / coef); }

// Coord **********************************************************************************************************************

// 2d integer coordinate
class Coord {
public:
	constexpr Coord(int in_x, int in_y) : m_x(in_x), m_y(in_y) {
	}
	constexpr Coord(int in_pos) : m_x(in_pos), m_y(in_pos) {
	}
	/*explicit Coord(const vec2& in_pos) :
			m_x(std::floor(in_pos.x())), m_y(std::floor(in_pos.y())) {
		}*/
	constexpr Coord() : m_x(0), m_y(0) {}
	constexpr inline void set(int in_x, int in_y) {
		this->m_x = in_x;
		this->m_y = in_y;
	}
	constexpr inline void setX(int in_x) { this->m_x = in_x; }
	constexpr inline void setY(int in_y) { this->m_y = in_y; }
	constexpr inline int getX() const { return this->m_x; }
	constexpr inline int x() const { return this->m_x; }
	constexpr inline int getY() const { return this->m_y; }
	constexpr inline int y() const { return this->m_y; }

	constexpr inline Coord operator+(const Coord& other) const { return Coord(this->m_x + other.m_x, this->m_y + other.m_y); }
	constexpr inline Coord operator-(const Coord& other) const { return Coord(this->m_x - other.m_x, this->m_y - other.m_y); }
	constexpr inline Coord operator+=(const Coord& other) {
		this->m_x += other.m_x;
		this->m_y += other.m_y;
		return *this;
	}
	constexpr inline Coord operator-=(const Coord& other) {
		this->m_x -= other.m_x;
		this->m_y -= other.m_y;
		return *this;
	}

	constexpr inline Coord operator=(const Coord& other) {
		this->m_x = other.m_x;
		this->m_y = other.m_y;
		return *this;
	}
	constexpr inline bool operator==(const Coord& other) const { return this->m_x == other.m_x && this->m_y == other.m_y; }
	constexpr inline bool operator!=(const Coord& other) const { return !(this->m_x == other.m_x && this->m_y == other.m_y); }
	constexpr inline bool operator<(const Coord& other) const { return this->m_x * this->m_x + this->m_y * this->m_y < other.m_x * other.m_x + other.m_y * other.m_y; }
	constexpr inline bool operator>(const Coord& other) const { return this->m_x * this->m_x + this->m_y * this->m_y > other.m_x * other.m_x + other.m_y * other.m_y; }

	constexpr inline vec2 operator*(float in) const { return vec2{m_x * in, m_y * in}; }

	constexpr inline bool valid(int edge) {
		return m_x >= 0 && m_y >= 0 && m_x < edge && m_y < edge;
	}

	constexpr inline Coord offset(int in_x, int in_y) const { return Coord{this->m_x + in_x, this->m_y + in_y}; }
	constexpr inline Coord offsetX(int in) const { return Coord(this->m_x + in, this->m_y); }
	constexpr inline Coord offsetY(int in) const { return Coord(this->m_x, this->m_y + in); }
	constexpr inline void move(int in_x, int in_y) {
		this->m_x += in_x;
		this->m_y += in_y;
	}
	constexpr inline void moveX(int in) { this->m_x += in; }
	constexpr inline void moveY(int in) { this->m_y += in; }

private:
	int m_x, m_y;
};

inline Coord toCoord(const vec2& in) { return Coord{static_cast<int>(std::floor(in.x())), static_cast<int>(std::floor(in.y()))}; }
inline vec2 toVec2(const Coord& in) { return vec2{static_cast<float>(in.x()), static_cast<float>(in.y())}; }

inline std::ostream& operator<<(std::ostream& in_cout, const vec2& in_vec2) {
	in_cout << "( " << in_vec2.x() << ", " << in_vec2.y() << " )";
	return in_cout;
}
inline std::ostream& operator<<(std::ostream& in_cout, const Coord& in_coord) {
	in_cout << "( " << in_coord.x() << ", " << in_coord.y() << " )";
	return in_cout;
}

// constants **************************************************************************************************************

constexpr const vec2 IHat(1.0f, 0.0f);
constexpr const vec2 JHat(0.0f, 1.0f);
constexpr const vec2 Origin(0.0f, 0.0f);
constexpr const vec2 InvalidVec(NAN, NAN);

constexpr vec2 TopRight = {1.0, 1.0};
constexpr vec2 TopLeft = {-1.0, 1.0};
constexpr vec2 BottomLeft = {-1.0, -1.0};
constexpr vec2 BottomRight = {1.0, -1.0};

constexpr float PI = 3.1415926f;
constexpr float ONE_DEGREE = 0.017453292f;

constexpr Coord _8wayIncrement[] = {
    {1, 0},
    {-1, 0},
    {0, 1},
    {0, -1},
    {1, 1},
    {-1, 1},
    {-1, -1},
    {1, -1}};
constexpr Coord _4wayIncrement[] = {
    {1, 0},
    {-1, 0},
    {0, 1},
    {0, -1}};
constexpr int _2wayIncrement[] = {
    1, -1};
constexpr Coord CoordOrigin{0, 0};














} // namespace tx