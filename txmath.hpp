// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// File: TXLib_Math

#pragma once
#include "txlib.hpp"

namespace tx {

	float findC(const vec2& p, const vec2& AB) { // C = -(Ax + By)
		return -dot(p, AB);
	}
	// Ax + By + C = 0
	class LineEquation {
	public:
		LineEquation(const vec2& pa, const vec2& pb) {
			AB = leftPerp(pa - pb);
			C = findC(pa, AB);
		}
		constexpr LineEquation(float in_A, float in_B, float in_C) : AB(in_A, in_B), C(in_C) {}
		constexpr LineEquation(const vec2& in_AB, float in_C) : AB(in_AB), C(in_C) {}
		constexpr LineEquation() {}

		inline const vec2& getAB() const { return AB; }
		inline const vec2& ab() const { return AB; }
		inline float a() const { return AB.x(); }
		inline float b() const { return AB.y(); }
		inline float getC() const { return C; }
		inline float c() const { return C; }

		inline void moveX(float offset) {
			C += AB.x() * -offset;
		}
		inline void moveY(float offset) {
			C += AB.y() * -offset;
		}
		inline void move(float offsetX, float offsetY) {
			C += AB.x() * -offsetX + AB.y() * -offsetY;
		}
		inline void move(const vec2& offset) {
			C += dot(AB, -offset);
		}
		inline LineEquation offsetX(float offset) const {
			return LineEquation{ AB, C + AB.x() * -offset };
		}
		inline LineEquation offsetY(float offset) const {
			return LineEquation{ AB, C + AB.y() * -offset };
		}
		inline LineEquation offset(float offsetX, float offsetY) const {
			return LineEquation{ AB, C + AB.x() * -offsetX + AB.y() * -offsetY };
		}
		inline LineEquation offset(const vec2& offset) const {
			return LineEquation{ AB, C + dot(AB, -offset) };
		}


	private:
		float C;
		vec2 AB;
	};
	using MathLine = LineEquation;
	constexpr MathLine x_axis = MathLine{ 0.0f, 1.0f, 0.0f };
	constexpr MathLine y_axis = MathLine{ 1.0f, 0.0f, 0.0f };


	inline MathLine findLineThruPoint(const vec2& p, const vec2& in_AB) { return MathLine{ in_AB, findC(p, in_AB) }; }

	inline float findLineY(const MathLine& l, float x) {
		return l.b() != 0.0f ? (l.a() * x + l.c()) / -l.b() : -l.c();
	}
	inline float findLineX(const MathLine& l, float y) {
		return l.a() != 0.0f ? (l.b() * y + l.c()) / -l.a() : -l.c();
	}
	inline bool isParallel(const MathLine& la, const MathLine& lb) {
		return la.a() * lb.b() == la.b() * lb.a();
	}
	/*inline bool isRightCross(const MathLine& la, const MathLine& lb) {
		return !(lb.b() * la.a() - la.b() * lb.a());
	}*/
	inline bool isHorizontal(const MathLine& l) { return l.a() == 0.0f; }
	inline bool isVertival(const MathLine& l) { return l.b() == 0.0f; }

	
	//inline vec2 findIntersection(const MathLine& l1, const MathLine& l2) {
	//	if (isParallel(l1, l2)) return InvalidVec;
	//	if (isRightCross(l1, l2)) {
	//		const MathLine& vertical = !l1.a() ? l1 : l2;
	//		const MathLine& horizontal = !l1.a() ? l2 : l1;
	//		return vec2(-vertical.c(), -horizontal.c());
	//	}
	//	float y = (l2.c() * l1.a() - l1.c() * l2.a()) / (l1.b() * l2.a() - l2.b() * l1.a()); // big bug: not add the negative sign at the front. probably because i messed up during solving the equation
	//	return vec2(findLineX(l1, y), y);
	//}

	inline vec2 findIntersection(const MathLine& l1, const MathLine& l2) {
		float det = l1.a() * l2.b() - l2.a() * l1.b(); // cross product of l1.AB and l2.AB; also the denominator of my old equation
		if (det == 0.0f) return InvalidVec;

		return vec2{
			(l1.b() * l2.c() - l2.b() * l1.c()) / det,
			(l2.a() * l1.c() - l1.a() * l2.c()) / det
		};
	}



	inline vec2 findMidPoint(const vec2& pa, const vec2& pb) {
		return pa + (pb - pa) * 0.5f;
	}

	inline MathLine offsetX(const MathLine& l, float offset) {
		return MathLine{ l.ab(), l.c() + l.a() * -offset };
	}
	inline MathLine offsetY(const MathLine& l, float offset) {
		return MathLine{ l.ab(), l.c() + l.b() * -offset };
	}
	inline MathLine offset(const MathLine& l, float offsetX, float offsetY) {
		return MathLine{ l.ab(), l.c() + l.a() * -offsetX + l.b() * -offsetY };
	}

	inline bool isRightOfLine(const MathLine& l, const vec2& p) {
		return p.x() > findLineX(l, p.y());
	}
	inline bool isLeftOfLine(const MathLine& l, const vec2& p) {
		return p.x() < findLineX(l, p.y());
	}
	inline bool isAboveLine(const MathLine& l, const vec2& p) {
		return p.y() > findLineY(l, p.x());
	}
	inline bool isBelowLine(const MathLine& l, const vec2& p) {
		return p.y() < findLineY(l, p.x());
	}


	class LineSegment {
	public:
		LineSegment(const vec2& in_start, const vec2& in_end) :
			m_line (in_start, in_end),
			m_start(_startX(in_start, in_end), _startY(in_start, in_end)),
			m_end  (_endX  (in_start, in_end), _endY  (in_start, in_end)) {}
		LineSegment(const MathLine& in_line, const vec2& in_start, const vec2& in_end) : 
			m_line(in_line), 
			m_start(_startX(in_start, in_end), _startY(in_start, in_end)),
			m_end  (_endX  (in_start, in_end), _endY  (in_start, in_end)) {}
				    
		inline const MathLine& line () const { return m_line;  }
		// bottomLeft
		inline const vec2&     start() const { return m_start; }
		// topRight
		inline const vec2&     end  () const { return m_end;   }

		inline float length() const { return std::hypotf(m_end.x() - m_start.x(), m_end.y() - m_start.x()); }

	private:
		MathLine m_line;
		vec2 m_start, m_end;

		static inline float _startX(const vec2& start, const vec2& end) { return min(start.x(), end.x()); }
		static inline float _startY(const vec2& start, const vec2& end) { return min(start.y(), end.y()); }
		static inline float _endX  (const vec2& start, const vec2& end) { return max(start.x(), end.x()); }
		static inline float _endY  (const vec2& start, const vec2& end) { return max(start.y(), end.y()); }


	};
	
	inline vec2 findIntersection(const MathLine& l, const LineSegment& ls) {
		vec2 intersection = findIntersection(l, ls.line());
		if (isValid(intersection) && inRange(intersection, ls.start(), ls.end())) {
			return intersection;
		} else {
			return InvalidVec;
		}
	}
	inline vec2 findIntersection(const LineSegment& ls1, const LineSegment& ls2) {
		vec2 intersection = findIntersection(ls1.line(), ls2.line());
		if (isValid(intersection) && inRange(intersection, ls1.start(), ls1.end()) && inRange(intersection, ls2.start(), ls2.end())) {
			return intersection;
		} else {
			return InvalidVec;
		}
	}

	class DLineSegment {
	public:
		DLineSegment() {}
		DLineSegment(const vec2& in_start, float in_dx, float in_dy) :
			m_start(in_start), m_dx(in_dx), m_dy(in_dy)
		{}
		DLineSegment(const vec2& in_start, const vec2& in_offsetVec) :
			m_start(in_start), m_dx(in_offsetVec.x()), m_dy(in_offsetVec.y())
		{}

		const vec2& start() const { return m_start; }
		vec2 end() const { return m_start.offset(m_dx, m_dy); }
		float dx() const { return m_dx; }
		float dy() const { return m_dy; }
		vec2 dVec() const { return vec2{ m_dx, m_dy }; }

		float findTviaX(float x) const { return (x - m_start.x()) / m_dx; }
		float findTviaY(float y) const { return (y - m_start.y()) / m_dy; }
		float findDX(float t) const { return t * m_dx; }
		float findDY(float t) const { return t * m_dy; }
		float findX(float t) const { return m_start.x() + findDX(t); }
		float findY(float t) const { return m_start.y() + findDY(t); }
		vec2 findPoint(float t) const { return vec2{ findX(t), findY(t) }; }
		float findXviaY(float y) const { return findX(findTviaY(y)); }
		float findYviaX(float x) const { return findY(findTviaX(x)); }

		void applyStartT(float t) { m_start.offset(m_dx * t, m_dy * t); m_dx *= (1 - t); m_dy *= (1 - t); }
		void applyEndT(float t) { m_dx *= t; m_dy *= t; }

		static bool validT(float t) { return inRange(t, 0.0f, 1.0f + epsilon); }
	private:
		vec2 m_start;
		float m_dx, m_dy;
	};

	using DLineSeg = DLineSegment;

	DLineSeg makeLineSegment(const vec2& in_start, const vec2& in_end) {
		return DLineSeg{ in_start, in_end - in_start };
	}
	










	struct Line {
		Line() {}
		Line(const vec2& in_pa, const vec2& in_pb) : pa(in_pa), pb(in_pb) {}
		Line(const MathLine& in_line) : pa(-1.0f, findLineY(in_line, -1.0f)), pb(1.0f, findLineY(in_line, 1.0f)) {}
		vec2 pa, pb;
	};



	vec2 selectShortest(const vec2& can1, const vec2& can2, const vec2& origin) {
		return (origin - can1) < (origin - can2) ? can1 : can2;
	}
	vec2 selectLongest(const vec2& can1, const vec2& can2, const vec2& origin) {
		return (origin - can1) > (origin - can2) ? can1 : can2;
	}









}