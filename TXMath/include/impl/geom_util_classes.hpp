// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXMath
// File: geom_util_classes.h

#pragma once
#include "impl/basic_utils.hpp"
#include "impl/geometry.hpp"
#include <concepts>

namespace tx {

template <std::invocable<const Coord&> Func>
inline void applyGridRange(Func&& f, const Coord& bottomLeft, uint32_t width, uint32_t height) {
	Coord cur = bottomLeft;
	Coord end = bottomLeft.offset(width, height);
	for (; cur.y < end.y; cur.y++) {
		for (cur.x = bottomLeft.x; cur.x < end.x; cur.x++) {
			f(cur);
		}
	}
}

class Rect {
public:
	Rect(const vec2& in_pos, float in_w, float in_h) : m_dimension(in_w, in_h), m_bottomLeft(in_pos) {}
	Rect(const vec2& in_pos, const vec2& diagonalVec)
	    : m_dimension(std::abs(diagonalVec.x), std::abs(diagonalVec.y)),
	      m_bottomLeft(vec2{
	          diagonalVec.x < 0.0f ? in_pos.x + diagonalVec.x : in_pos.x,
	          diagonalVec.y < 0.0f ? in_pos.y + diagonalVec.y : in_pos.y }) {}

	inline vec2 dimension() const { return m_dimension; }
	inline vec2 size() const { return m_dimension; }

	inline vec2 topRight() const { return m_bottomLeft + m_dimension; }
	inline vec2 topLeft() const { return m_bottomLeft + vec2(0.0f, m_dimension.y); }
	inline vec2 bottomRight() const { return m_bottomLeft + vec2(m_dimension.x, 0.0f); }
	inline vec2 bottomLeft() const { return m_bottomLeft; }
	inline vec2 center() const { return m_bottomLeft + m_dimension * 0.5f; }

	inline float left() const { return m_bottomLeft.x; }
	inline float right() const { return m_bottomLeft.x + m_dimension.x; }
	inline float top() const { return m_bottomLeft.y + m_dimension.y; }
	inline float bottom() const { return m_bottomLeft.y; }

	inline float width() const { return m_dimension.x; }
	inline float height() const { return m_dimension.y; }

	template <std::invocable<const Coord&> Func>
	inline void apply(Func&& f) const {
		u32 w = static_cast<u32>(std::floorf(m_bottomLeft.x + m_dimension.x) - std::floorf(m_bottomLeft.x)) + (isInt(m_dimension.x) ? 0 : 1),
		    h = static_cast<u32>(std::floorf(m_bottomLeft.y + m_dimension.y) - std::floorf(m_bottomLeft.y)) + (isInt(m_dimension.y) ? 0 : 1);
		applyGridRange(std::forward<Func>(f), toCoord(m_bottomLeft), w, h);
	}

	inline void move(vec2 movement) { m_bottomLeft += movement; }
	inline void moveX(float in) { m_bottomLeft.x += in; }
	inline void moveY(float in) { m_bottomLeft.y += in; }

	inline Rect offset(vec2 offset) const { return Rect{ m_bottomLeft + offset, m_dimension }; }
	inline Rect offsetX(float in) const { return Rect{ m_bottomLeft.offsetX(in), m_dimension }; }
	inline Rect offsetY(float in) const { return Rect{ m_bottomLeft.offsetY(in), m_dimension }; }

	inline void setPos(vec2 pos) { m_bottomLeft = pos; }
	inline void setPos(float x, float y) {
		m_bottomLeft.x = x;
		m_bottomLeft.y = y;
	}

private:
	vec2 m_dimension;
	vec2 m_bottomLeft;
};
inline Rect makeRange(const vec2& in_bottomLeft, const vec2& in_topRight) {
	return Rect{ in_bottomLeft, in_topRight - in_bottomLeft };
}

// inclusive-inclusive
class DiscreteRect {
public:
	DiscreteRect(const Coord& in_bottomLeft, int in_w, int in_h) : m_bottomLeft(in_bottomLeft), m_width(in_w), m_height(in_h) {}
	// inclusive-inclusive
	DiscreteRect(const Coord& start, const Coord& end)
	    : m_bottomLeft(Coord{
	          min(start.x, end.x),
	          min(start.y, end.y) }),
	      m_width(std::abs(start.x - end.x + 1)), m_height(std::abs(start.y - end.y + 1)) {}

	Coord topRight() const { return Coord{ m_bottomLeft.x + m_width - 1, m_bottomLeft.y + m_height - 1 }; }
	Coord topLeft() const { return Coord{ m_bottomLeft.x, m_bottomLeft.y + m_height - 1 }; }
	Coord bottomRight() const { return Coord{ m_bottomLeft.x + m_width - 1, m_bottomLeft.y }; }
	Coord bottomLeft() const { return m_bottomLeft; }
	Coord center() const { return Coord{ static_cast<int>(m_bottomLeft.x + m_width * 0.5), static_cast<int>(m_bottomLeft.y + m_height * 0.5) }; }

	u32 width() const { return m_width; }
	u32 height() const { return m_height; }

	template <std::invocable<const Coord&> Func>
	inline void apply(Func&& f) const {
		applyGridRange(std::forward<Func>(f), m_bottomLeft, m_width, m_height);
	}

private:
	Coord m_bottomLeft; // inclusive
	int m_width, m_height;
};
using DRect = DiscreteRect;
} // namespace tx