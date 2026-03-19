// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGrid
// File: grid_utils.cpp

#include "impl/grid_utils.h"

namespace tx {

// ******** GridCircle ********

GridCircle::GridCircle(float in_radius) {
	float rsq = sq(in_radius);
	this->radius = std::ceil(in_radius - 0.5f);
	int sectorRange = this->radius - 1;
	float height = 0.5f;
	for (int i = 0; i < sectorRange; i++) {
		//sector.push_back(std::cos(std::asinf(height / in_radius)) * in_radius)
		sector.push_back(std::ceil(std::sqrtf(rsq - sq(height)) - 0.5f));
		height += 1.0f;
	}
	sector.push_back(std::ceil(std::sqrtf(rsq - sq(height)) - 0.5f));
}
CoordMap GridCircle::getCoordMap(const Coord& center) const {
	CoordMap buffer;
	buffer.reserve(getGridAmount());
	gen_impl(center, ApplyToCoordMap_impl{ &buffer });
	buffer.sort();
	return buffer;
}
void GridCircle::applyToCoordMap(const Coord& center, CoordMap& cm) const {
	size_t oldSize = cm.size();
	gen_impl(center, ApplyToCoordMap_impl{ &cm });
	cm.sort(cm.begin() + oldSize, cm.end());
	std::inplace_merge(cm.begin(), cm.begin() + oldSize, cm.end());
}
template void GridCircle::gen_impl<GridCircle::ApplyToCoordMap_impl>(const Coord&, GridCircle::ApplyToCoordMap_impl&&) const;

// ******** GridLine ********

inline void GridLine::reinit(const vec2& in_start, const vec2& in_end, int in_width, int in_height) {
	width = in_width;
	height = in_height;
	rangeEnd = Coord{ in_width, in_height };
	topRight = toVec2(rangeEnd);
	line = makeLineSegment(in_start, in_end);

	init_impl(in_start, in_end);
}

void GridLine::init_impl(const vec2& in_start, const vec2& in_end) {
	direction = find8wayDir(line.dVec());
	directionIncrement = [&]() -> Coord {
		return Coord{
			direction.x() > 0 ? 1 : 0,
			direction.y() > 0 ? 1 : 0
		};
	}();
	findStartEnd_impl(in_start, in_end);

	//passedGrids.sort();
}

void GridLine::findStartEnd_impl(const vec2& in_start, const vec2 in_end) {
	vec2 startEndT = liang_findInRangeStartEndT(in_start, in_end, Origin, topRight);
	this->start = toCoord(line.findPoint(startEndT.x()) + toVec2(direction) * epsilon * 3.0f); // 3.0f is just a magic number to make the epsilon bigger
	this->end = toCoord(line.findPoint(startEndT.y()));
	startT = startEndT.x();
	endT = startEndT.y();
	direction = find8wayDir(end - start);
	//passedGrids.push_back(current);
	//passedGrids.push_back(end);
}
float GridLine::findNextCoord_impl(Coord& current) const {
	float nextxt = line.findTviaX(static_cast<float>(current.x() + directionIncrement.x()));
	float nextyt = line.findTviaY(static_cast<float>(current.y() + directionIncrement.y()));
	if (nextyt > nextxt) {
		current.moveX(direction.x());
		return nextxt;
	} else if (nextyt < nextxt) {
		current.moveY(direction.y());
		return nextyt;
	} else {
		current += direction;
		return nextxt;
	}
	//current = toCoord(line.findPoint(min(nextyt, nextxt)));
}


bool GridLine::liang_solveOneConstraint(float p, float q, float& tStart, float& tEnd) {
	if (p == 0.0f) return q >= 0.0f;

	float tMeet = q / p;

	if (p > 0.0f) {
		if (tMeet < tStart) return false;
		if (tMeet < tEnd) tEnd = tMeet;
	} else { // p < 0.0f
		if (tMeet > tEnd) return false;
		if (tMeet > tStart) tStart = tMeet;
	}
	return true;
}
vec2 GridLine::liang_findInRangeStartEndT(const vec2& start, const vec2 end, const vec2& rangeBottomLeft, const vec2& rangeTopRight) {
	vec2 dVec = end - start;
	float dx = dVec.x();
	float dy = dVec.y();

	const vec2& min = rangeBottomLeft;
	const vec2& max = rangeTopRight;

	float tStart = 0.0f;
	float tEnd = 1.0f;

	// order don't matter
	if (!liang_solveOneConstraint(-dx, start.x() - min.x(), tStart, tEnd)) { return InvalidVec; } // left
	if (!liang_solveOneConstraint(-dy, start.y() - min.y(), tStart, tEnd)) { return InvalidVec; } // bottom
	if (!liang_solveOneConstraint(dx, max.x() - start.x(), tStart, tEnd)) { return InvalidVec; } // right
	if (!liang_solveOneConstraint(dy, max.y() - start.y(), tStart, tEnd)) { return InvalidVec; } // top

	return vec2{ tStart, tEnd };
}

template void GridLine::gen_impl<GridLine::ApplyToCoordMap_impl>(GridLine::ApplyToCoordMap_impl&&) const;
template void GridLine::marchGrids_impl<GridLine::ApplyToCoordMap_impl>(GridLine::ApplyToCoordMap_impl&&, Coord&) const;
template void GridLine::marchStraight_impl<GridLine::ApplyToCoordMap_impl>(GridLine::ApplyToCoordMap_impl&&, Coord&) const;
template void GridLine::gen_impl<GridLine::ApplyToCoordMapOffset_impl>(GridLine::ApplyToCoordMapOffset_impl&&) const;
template void GridLine::marchGrids_impl<GridLine::ApplyToCoordMapOffset_impl>(GridLine::ApplyToCoordMapOffset_impl&&, Coord&) const;
template void GridLine::marchStraight_impl<GridLine::ApplyToCoordMapOffset_impl>(GridLine::ApplyToCoordMapOffset_impl&&, Coord&) const;


} // namespace tx