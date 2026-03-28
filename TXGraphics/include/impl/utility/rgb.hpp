// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: rgb.hpp

#pragma once
#include <cmath>
#include <vector>
#include "tx/math.h"

namespace tx {

// Color and Rainbow **********************************************************************************************************
#ifdef RGB
#undef RGB
#endif
class RGB {
public:
	constexpr RGB(float in_r, float in_g, float in_b) : m_r(in_r), m_g(in_g), m_b(in_b) {
	}
	constexpr RGB() : m_r{}, m_g{}, m_b{} {
	}

	constexpr inline const float r() const { return this->m_r; }
	constexpr inline const float g() const { return this->m_g; }
	constexpr inline const float b() const { return this->m_b; }

	inline bool operator==(const tx::RGB& other) const {
		return this->m_r == other.m_r && this->m_g == other.m_g && this->m_b == other.m_b;
	}
	inline bool operator!=(const tx::RGB& other) const { return !this->operator==(other); }

	inline void normalize() {
		m_r *= oneOf255;
		m_g *= oneOf255;
		m_b *= oneOf255;
	}
	inline RGB normalized() const {
		return RGB{
			m_r * oneOf255,
			m_g * oneOf255,
			m_b * oneOf255
		};
	}
	inline bool isNormalized() const {
		return m_r <= 1.0f &&
		       m_g <= 1.0f &&
		       m_b <= 1.0f;
	}
	inline void denormalize() {
		m_r *= 255.0f;
		m_g *= 255.0f;
		m_b *= 255.0f;
	}
	inline RGB denormalized() const {
		return RGB{
			m_r * 255.0f,
			m_g * 255.0f,
			m_b * 255.0f
		};
	}

	inline u32 compress() const {
		RGB temp = isNormalized() ? denormalized() : *this;
		u32 r8 = static_cast<u32>(temp.m_r) & 0xFF;
		u32 g8 = static_cast<u32>(temp.m_g) & 0xFF;
		u32 b8 = static_cast<u32>(temp.m_b) & 0xFF;
		u32 a8 = 0xFF;
		return (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
	}

private:
	float m_r, m_g, m_b;
};

class RGBA {
public:
	constexpr RGBA(float in_r, float in_g, float in_b, float in_a) : m_r(in_r), m_g(in_g), m_b(in_b), m_a(in_a) {}
	constexpr RGBA(const RGB& in_rgb, float in_a) : m_r(in_rgb.r()), m_g(in_rgb.g()), m_b(in_rgb.b()), m_a(in_a) {}
	constexpr RGBA() : m_r{}, m_g{}, m_b{}, m_a{} {}

	constexpr inline float r() const { return m_r; }
	constexpr inline float g() const { return m_g; }
	constexpr inline float b() const { return m_b; }
	constexpr inline float a() const { return m_a; }

	inline void setR(float in_r) { m_r = in_r; }
	inline void setG(float in_g) { m_g = in_g; }
	inline void setB(float in_b) { m_b = in_b; }
	inline void setA(float in_a) { m_a = in_a; }

	inline bool operator==(const tx::RGBA& other) const {
		return m_r == other.m_r && m_g == other.m_g && m_b == other.m_b && m_a == other.m_a;
	}
	inline bool operator!=(const tx::RGBA& other) const { return !this->operator==(other); }

	inline void normalize() {
		m_r *= oneOf255;
		m_g *= oneOf255;
		m_b *= oneOf255;
		m_a *= oneOf255;
	}
	inline RGBA normalized() const {
		return RGBA{
			m_r * oneOf255,
			m_g * oneOf255,
			m_b * oneOf255,
			m_a * oneOf255
		};
	}
	inline bool isNormalized() const {
		return m_r <= 1.0f && m_g <= 1.0f && m_b <= 1.0f && m_a <= 1.0f;
	}
	inline void denormalize() {
		m_r *= 255.0f;
		m_g *= 255.0f;
		m_b *= 255.0f;
		m_a *= 255.0f;
	}
	inline RGBA denormalized() const {
		return RGBA{
			m_r * 255.0f,
			m_g * 255.0f,
			m_b * 255.0f,
			m_a * 255.0f
		};
	}

	inline u32 compress() const {
		RGBA temp = isNormalized() ? denormalized() : *this;
		u32 r8 = static_cast<u32>(temp.m_r) & 0xFF;
		u32 g8 = static_cast<u32>(temp.m_g) & 0xFF;
		u32 b8 = static_cast<u32>(temp.m_b) & 0xFF;
		u32 a8 = static_cast<u32>(temp.m_a) & 0xFF;
		return (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
	}

private:
	float m_r, m_g, m_b, m_a;
};

RGB InvalidColor = RGB(-1, -1, -1);
constexpr RGB Black = RGB{};
constexpr RGB White = RGB(255, 255, 255);

constexpr RGB Red = RGB(255, 0, 0);
constexpr RGB Green = RGB(0, 255, 0);
constexpr RGB Blue = RGB(0, 0, 255);

constexpr RGB Yellow = RGB(255, 255, 0);
constexpr RGB Cyan = RGB(0, 255, 255);
constexpr RGB Magenta = RGB(255, 0, 255);

constexpr RGB Gray = RGB(128, 128, 128);
constexpr RGB DarkGray = RGB(64, 64, 64);
constexpr RGB LightGray = RGB(192, 192, 192);

constexpr RGB Orange = RGB(255, 165, 0);
constexpr RGB Purple = RGB(128, 0, 128);
constexpr RGB Brown = RGB(139, 69, 19);
constexpr RGB Pink = RGB(255, 192, 203);

constexpr RGB DarkRed = RGB(139, 0, 0);
constexpr RGB LightRed = RGB(255, 102, 102);
constexpr RGB DarkGreen = RGB(0, 100, 0);
constexpr RGB LightGreen = RGB(144, 238, 144);
constexpr RGB DarkBlue = RGB(0, 0, 139);
constexpr RGB LightBlue = RGB(173, 216, 230);

constexpr RGB SkyBlue = RGB(135, 206, 235);
constexpr RGB SteelBlue = RGB(70, 130, 180);
constexpr RGB Navy = RGB(0, 0, 128);
constexpr RGB Gold = RGB(255, 215, 0);
constexpr RGB Khaki = RGB(240, 230, 140);
constexpr RGB Olive = RGB(128, 128, 0);
constexpr RGB Coral = RGB(255, 127, 80);
constexpr RGB Salmon = RGB(250, 128, 114);
constexpr RGB Tomato = RGB(255, 99, 71);

constexpr RGB MikuColor = RGB(0, 210, 255);
constexpr RGB MikuColorBegin = RGB(0, 230, 255);
constexpr RGB MikuColorEnd = RGB(0, 255, 200);


class Rainbow {
public:
	Rainbow(int range) { // maybe more rainbow generating algorithms?
		float increment = PI / range,
		      third = PI / 3;
		for (int i = 0; i < range; i++) {
			m_rainbow.push_back(RGB(
			    std::abs(std::sinf(increment * i)),
			    std::abs(std::sinf(increment * i + third)),
			    std::abs(std::sinf(increment * i + third * 2))));
		}
	}

	inline const RGB& operator[](int index) const {
		return m_rainbow[index];
	}
	inline const RGB& getNextColor() {
		index++;
		if (index == this->m_rainbow.size()) {
			index = 0;
		}
		return m_rainbow[index];
	}

private:
	std::vector<RGB> m_rainbow;
	int index = -1;
};
} // namespace tx