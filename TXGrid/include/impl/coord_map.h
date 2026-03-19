// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGrid
// File: coord_map.h

#pragma once
#include "impl/geom_util_classes.h"
#include "impl/geom_utils.h"
#include "impl/geometry.h"
#include <algorithm>

namespace tx {

// all coordmap must be sorted
class CoordMap {
public:
	using It_t = std::vector<Coord>::iterator;
	using ConstIt_t = std::vector<Coord>::const_iterator;

	void sort();
	void sort(It_t begin, It_t end);
	void clamp(const Coord& bottomLeft, const Coord& topRight);

	void append(const Coord& in) { m_coords.push_back(in); }
	void append(const CoordMap& in);

	void unique() { m_coords.erase(std::unique(m_coords.begin(), m_coords.end()), m_coords.end()); }

	class Compare {
	public:
		bool operator()(const Coord& a, const Coord& b);
	};

public:
	It_t begin() { return m_coords.begin(); }
	It_t end() { return m_coords.end(); }

	ConstIt_t begin() const { return m_coords.begin(); }
	ConstIt_t end() const { return m_coords.end(); }

	std::size_t size() const { return m_coords.size(); }
	const Coord& operator[](std::size_t i) const { return m_coords[i]; }
	Coord& operator[](std::size_t i) { return m_coords[i]; }
	void clear() { m_coords.clear(); }
	void reserve(int count) { m_coords.reserve(count); }
	Coord& front() { return m_coords.front(); }
	Coord& back() { return m_coords.back(); }
	void pop_back() { m_coords.pop_back(); }

private:
	std::vector<Coord> m_coords;
};
} // namespace tx