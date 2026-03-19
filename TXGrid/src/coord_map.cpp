// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGrid
// File: coord_map.cpp

#include "impl/coord_map.h"

namespace tx {

void CoordMap::sort() {
	std::sort(m_coords.begin(), m_coords.end(), Compare{});
	unique();
}
void CoordMap::sort(CoordMap::It_t begin, CoordMap::It_t end) {
	std::sort(begin, end, Compare{});
	unique();
}
void CoordMap::clamp(const Coord& bottomLeft, const Coord& topRight) {
	m_coords.erase(std::remove_if(m_coords.begin(), m_coords.end(), [&](const Coord& in) {
		return tx::inRange(in, bottomLeft, topRight);
	}));
}
void CoordMap::append(const CoordMap& in) {
	size_t oldSize = m_coords.size();
	m_coords.reserve(oldSize + in.size());
	m_coords.insert(m_coords.end(), in.begin(), in.end());
	std::inplace_merge(m_coords.begin(), m_coords.begin() + oldSize, m_coords.end(), Compare{});
	unique();
}
bool CoordMap::Compare::operator()(const Coord& a, const Coord& b) {
	if (a.y() == b.y()) {
		return a.x() < b.x();
	} else {
		return a.y() < b.y();
	}
}
} // namespace tx
