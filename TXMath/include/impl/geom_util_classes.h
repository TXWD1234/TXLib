// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXMath
// File: geom_util_classes.h

#pragma once
#include "impl/basic_utils.h"
#include "impl/geometry.h"

namespace tx {

template <class Func>
inline void applyGridRange(Func f, const Coord& bottomLeft, uint32_t width, uint32_t height) {
	static_assert(std::is_invocable_v<Func, const Coord&>,
	              "tx::applyGridRange(): invalid lambda callback parameter. The lambda provided need to have parameter of: (const tx::Coord&)");
	Coord cur = bottomLeft;
	Coord end = bottomLeft.offset(width, height);
	for (; cur.y() < end.y(); cur.moveY(1)) {
		for (cur.setX(bottomLeft.x()); cur.x() < end.x(); cur.moveX(1)) {
			f(cur);
		}
	}
}

class Rect {
public:
	Rect(const vec2& in_pos, float in_w, float in_h) : m_bottomLeft(in_pos), m_width(in_w), m_height(in_h) {}
	Rect(const vec2& in_pos, const vec2& diagonalVec)
	    : m_width(std::abs(diagonalVec.x())), m_height(std::abs(diagonalVec.y())),
	      m_bottomLeft(vec2{
	          diagonalVec.x() < 0.0f ? in_pos.x() + diagonalVec.x() : in_pos.x(),
	          diagonalVec.y() < 0.0f ? in_pos.y() + diagonalVec.y() : in_pos.y() }) {}

	inline vec2 topRight() const { return m_bottomLeft + vec2(m_width, m_height); }
	inline vec2 topLeft() const { return m_bottomLeft + vec2(0.0f, m_height); }
	inline vec2 bottomRight() const { return m_bottomLeft + vec2(m_width, 0.0f); }
	inline vec2 bottomLeft() const { return m_bottomLeft; }
	inline vec2 center() const { return m_bottomLeft + vec2(m_width * 0.5f, m_height * 0.5f); }

	inline float width() const { return m_width; }
	inline float height() const { return m_height; }

	template <class Func>
	inline void apply(Func&& f) const {
		u32 w = static_cast<u32>(std::floorf(m_bottomLeft.x() + m_width) - std::floorf(m_bottomLeft.x())) + (isInt(m_width) ? 0 : 1),
		    h = static_cast<u32>(std::floorf(m_bottomLeft.y() + m_height) - std::floorf(m_bottomLeft.y())) + (isInt(m_height) ? 0 : 1);
		applyGridRange(std::forward<Func>(f), toCoord(m_bottomLeft), w, h);
	}

private:
	vec2 m_bottomLeft;
	float m_width, m_height;
};
inline Rect makeRange(const vec2& in_bottomLeft, const vec2& in_topRight) {
	return Rect{ in_bottomLeft, in_topRight - in_bottomLeft };
}

// inclusive-inclusive
class DiscreteRect {
public:
	DiscreteRect(const Coord& in_bottomLeft, int in_w, int in_h) : m_bottomLeft(in_bottomLeft), m_width(in_w), m_height(in_h) {}
	// inclusive-inclusive
	DiscreteRect(const Coord& start, const Coord& end) // inclusive-inclusive
	    : m_width(std::abs(start.x() - end.x() + 1)), m_height(std::abs(start.y() - end.y() + 1)),
	      m_bottomLeft(Coord{
	          min(start.x(), end.x()),
	          min(start.y(), end.y()) }) {}

	Coord topRight() const { return Coord{ m_bottomLeft.x() + m_width - 1, m_bottomLeft.y() + m_height - 1 }; }
	Coord topLeft() const { return Coord{ m_bottomLeft.x(), m_bottomLeft.y() + m_height - 1 }; }
	Coord bottomRight() const { return Coord{ m_bottomLeft.x() + m_width - 1, m_bottomLeft.y() }; }
	Coord bottomLeft() const { return m_bottomLeft; }
	Coord center() const { return Coord{ static_cast<int>(m_bottomLeft.x() + m_width * 0.5), static_cast<int>(m_bottomLeft.y() + m_height * 0.5) }; }

	u32 width() const { return m_width; }
	u32 height() const { return m_height; }

	template <class Func>
	inline void apply(Func&& f) const {
		applyGridRange(std::forward<Func>(f), m_bottomLeft, m_width, m_height);
	}

private:
	Coord m_bottomLeft; // inclusive
	int m_width, m_height;
};
using DRect = DiscreteRect;
} // namespace tx