// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGrid
// File: grid_utils.h

#pragma once
#include "impl/coord_map.h"
#include "impl/geom_utils.h"
#include "impl/geometry.h"
#include "impl/grid_system.h"
#include "impl/math_line.h"
#include <vector>

namespace tx {
class GridCircle {
public:
	GridCircle(float in_radius);

	template <class Func>
	void apply(const Coord& center, Func&& f) const {
		static_assert(
		    std::is_invocable_v<Func, Coord&>,
		    "[Error]: tx::GridCircle::apply: invalid lambda parameter. The callback function provided need to have a parameter of tx::Coord.");
		gen_impl(center, std::forward<Func>(f));
	}

	CoordMap getCoordMap(const Coord& center) const;
	void applyToCoordMap(const Coord& center, CoordMap& cm) const;

	template <class Func, class T>
	void applyToGridSys(const Coord& center, GridSystem<T>& gs, Func&& f) const {
		static_assert(
		    std::is_invocable_v<Func, T&> || std::is_invocable_v<Func, T&, Coord&>,
		    "[Error]: tx::GridCircle::applyToGridSys: invalid lambda parameter. The callback function provided need to have a parameter of tx::Coord or parameters of (T, tx::Coord).");
		gen_impl(center, [&](const Coord& pos) {
			if constexpr (std::is_invocable_v<Func, T&, Coord&>) {
				f(gs.at(pos), pos);
			} else {
				f(gs.at(pos));
			}
		});
	}

	// approximate
	int getGridAmount() const {
		return std::ceil(sq(this->radius) * PI);
		//return 4 * (sq(this->radius) - this->radius) + 1;
	}

private:
	std::vector<int> sector; // the x offset for each row from the center
	int radius;

	struct ApplyToCoordMap_impl {
		CoordMap* cm;
		void operator()(const Coord& c) const {
			cm->append(c);
		}
	};

	template <class Func>
	void gen_impl(const Coord& center, Func&& operation) const {

		operation(center);
		for (int j = 0; j < 2; j++) {
			Coord temp = center;
			for (int i = 0; i < this->radius; i++) {
				temp += _4wayIncrement[j];
				operation(temp);
			}
		}

		for (int row = 0; row < this->sector.size(); row++) {
			int rowOffset = this->sector[row];
			for (int i = -rowOffset; i <= rowOffset; i++) {
				operation(center + Coord(i, row + 1));
				operation(center + Coord(i, -(row + 1)));
			}
		}
	}
};

extern template void GridCircle::gen_impl<GridCircle::ApplyToCoordMap_impl>(const Coord&, GridCircle::ApplyToCoordMap_impl&&) const;

class GridLine {
	// terminology:
	// OFB = Out of Bound
public:
	GridLine() {}
	GridLine(const vec2& in_start, const vec2& in_end, int in_width, int in_height)
	    : width(in_width), height(in_height), topRight(in_width, in_height), rangeEnd(in_width, in_height), line(makeLineSegment(in_start, in_end)) {
		init_impl(in_start, in_end);
	}

	template <class Func>
	void apply(Func&& f) const {
		static_assert(
		    std::is_invocable_v<Func, const Coord&>,
		    "[Error]: tx::GridLine::apply: invalid lambda parameter. "
		    "The callback must accept parameter: (tx::Coord).");
		gen_impl(std::forward<Func>(f));
	}

	CoordMap getCoordMap() const {
		CoordMap cm;
		cm.reserve(static_cast<int>(line.dVec().length() * 2));
		gen_impl(ApplyToCoordMap_impl{ &cm });
		cm.sort();
		return cm;
	}
	CoordMap getCoordMap(const Coord& offset) const {
		CoordMap cm;
		cm.reserve(static_cast<int>(line.dVec().length() * 2));
		gen_impl(ApplyToCoordMapOffset_impl{ &cm, offset });
		cm.sort();
		return cm;
	}
	void applyToCoordMap(CoordMap& cm) const {
		size_t oldSize = cm.size();
		gen_impl(ApplyToCoordMap_impl{ &cm });
		cm.sort(cm.begin() + oldSize, cm.end());
		std::inplace_merge(cm.begin(), cm.begin() + oldSize, cm.end());
	}
	void applyToCoordMap(CoordMap& cm, const Coord& offset) const {
		size_t oldSize = cm.size();
		gen_impl(ApplyToCoordMapOffset_impl{ &cm, offset });
		cm.sort(cm.begin() + oldSize, cm.end());
		std::inplace_merge(cm.begin(), cm.begin() + oldSize, cm.end());
	}
	template <class Func, class T>
	void applyToGridSys(GridSystem<T>& gs, Func&& f) {
		static_assert(
		    std::is_invocable_v<Func, T&> || std::is_invocable_v<Func, T&, const Coord&>,
		    "[Error]: tx::GridLine::applyToGridSys: invalid lambda parameter. The callback function provided need to have a parameter of tx::Coord or parameters of (T, tx::Coord).");
		gen_impl([&](const Coord& pos) {
			if constexpr (std::is_invocable_v<Func, T&, const Coord&>) {
				f(gs.at(pos), pos);
			} else {
				f(gs.at(pos));
			}
		});
	}
	template <class Func, class T>
	void applyToGridSys(GridSystem<T>& gs, Func&& f, const Coord& offset) const {
		static_assert(
		    std::is_invocable_v<Func, T&> || std::is_invocable_v<Func, T&, const Coord&>,
		    "[Error]: tx::GridLine::applyToGridSys: invalid lambda parameter. The callback function provided need to have a parameter of tx::Coord or parameters of (T, tx::Coord).");
		gen_impl([&](Coord pos) {
			pos += offset;
			if constexpr (std::is_invocable_v<Func, T&, const Coord&>) {
				f(gs.at(pos), pos);
			} else {
				f(gs.at(pos));
			}
		});
	}


	void reinit(const vec2& in_start, const vec2& in_end, int in_width, int in_height);

private:
	struct ApplyToCoordMap_impl {
		CoordMap* cm;
		void operator()(const Coord& c) const {
			cm->append(c);
		}
	};
	struct ApplyToCoordMapOffset_impl {
		CoordMap* cm;
		Coord offset;
		void operator()(const Coord& c) const {
			cm->append(c + offset);
		}
	};

private:
	//CoordMap passedGrids;
	int width, height;
	vec2 topRight;
	DLineSeg line;
	Coord start, end;
	Coord direction;
	Coord directionIncrement; // just a offset to make grid borders aline with consecutive space
	Coord rangeEnd;
	float startT, endT;
	bool valid = 1;
	int maxMarchAmount;

	void init_impl(const vec2& in_start, const vec2& in_end);
	void findStartEnd_impl(const vec2& in_start, const vec2 in_end);

	template <class Func>
	void gen_impl(Func&& f) const;

	float findNextCoord_impl(Coord& current) const;
	template <class Func>
	void marchGrids_impl(Func&& f, Coord& current) const;
	template <class Func>
	void marchStraight_impl(Func&& f, Coord& current) const;

	bool liang_solveOneConstraint(float p, float q, float& tStart, float& tEnd);
	vec2 liang_findInRangeStartEndT(const vec2& start, const vec2 end, const vec2& rangeBottomLeft, const vec2& rangeTopRight);
};

template <class Func>
void GridLine::gen_impl(Func&& f) const {
	Coord current = start;
	if (current == end) {
		if (inRange(current, CoordOrigin, rangeEnd)) f(current);
	} else if (current.x() == end.x() || current.y() == end.y()) {
		//cout << "march straight\n";
		marchStraight_impl(std::forward<Func>(f), current);
	} else {
		//cout << "march grids\n";
		marchGrids_impl(std::forward<Func>(f), current);
	}
}

template <class Func>
void GridLine::marchGrids_impl(Func&& f, Coord& current) const {
	float t = startT;
	while (!inRange(current, CoordOrigin, rangeEnd)) t = findNextCoord_impl(current);
	while (t < endT) {
		//std::cout << line.findPoint(t) << ", " << t << ", " << current << '\n';
		f(current);
		// according to the current t, the `current` grid is **the grid which that t enters**
		// which means the line haven't march throught the `current` grid yet, t is just on the border of it
		t = findNextCoord_impl(current);
	}
	//passedGrids.append(current);
	//while (!inRange(passedGrids.back(), CoordOrigin, rangeEnd)) passedGrids.pop_back();
}
template <class Func>
void GridLine::marchStraight_impl(Func&& f, Coord& current) const {
	while (!inRange(current, CoordOrigin, rangeEnd)) current += direction;
	//cout << "current: " << current << " end: " << end << " direction: " << direction << '\n';
	while (current != end) {
		f(current);
		current += direction;
	}
	f(current);
}

extern template void GridLine::gen_impl<GridLine::ApplyToCoordMap_impl>(GridLine::ApplyToCoordMap_impl&&) const;
extern template void GridLine::marchGrids_impl<GridLine::ApplyToCoordMap_impl>(GridLine::ApplyToCoordMap_impl&&, Coord&) const;
extern template void GridLine::marchStraight_impl<GridLine::ApplyToCoordMap_impl>(GridLine::ApplyToCoordMap_impl&&, Coord&) const;
extern template void GridLine::gen_impl<GridLine::ApplyToCoordMapOffset_impl>(GridLine::ApplyToCoordMapOffset_impl&&) const;
extern template void GridLine::marchGrids_impl<GridLine::ApplyToCoordMapOffset_impl>(GridLine::ApplyToCoordMapOffset_impl&&, Coord&) const;
extern template void GridLine::marchStraight_impl<GridLine::ApplyToCoordMapOffset_impl>(GridLine::ApplyToCoordMapOffset_impl&&, Coord&) const;

} // namespace tx
