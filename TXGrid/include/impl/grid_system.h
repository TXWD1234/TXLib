// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGrid
// File: grid_system.h

#pragma once
#include "impl/basic_utils.h"
#include "impl/coord_map.h"
#include "impl/geometry.h"
#include <type_traits>
#include <vector>

namespace tx {

template <class T>
class GridSystem {
public:
	GridSystem() {}
	GridSystem(int in_SideLen) : dimension(in_SideLen, in_SideLen) {
		this->map.assign(sq(in_SideLen), T{});
	}
	GridSystem(int in_width, int in_height) : dimension(in_width, in_height) {
		this->map.assign(in_width * in_height, T{});
	}
	GridSystem(const Coord& in_dimension) : dimension(in_dimension) {
		this->map.assign(in_dimension.x() * in_dimension.y(), T{});
	}

	void fill(const CoordMap& coords, const T& val) {
		for (const Coord& i : coords) {
			this->at(i) = val;
		}
	}
	void fillSafe(const CoordMap& coords, const T& val) {
		for (const Coord& i : coords) {
			if (this->valid(i))
				this->at(i) = val;
		}
	}
	void clear(const T& val = T{}) {
		std::fill(this->map.begin(), this->map.end(), val);
	}



	inline T& atIndex(int index) { return this->map[index]; }
	inline const T& atIndex(int index) const { return this->map[index]; }
	inline T& at(const tx::Coord& pos) { return this->at(pos.x(), pos.y()); }
	inline const T& at(const tx::Coord& pos) const { return this->at(pos.x(), pos.y()); }
	inline T& at(int x, int y) { return this->map[index(x, y)]; }
	inline const T& at(int x, int y) const { return this->map[index(x, y)]; }
	inline T* atSafe(const tx::Coord& pos) { return this->atSafe(pos.x(), pos.y()); }
	inline const T* atSafe(const tx::Coord& pos) const { return this->atSafe(pos.x(), pos.y()); }
	inline T* atSafe(int x, int y) {
		if (valid(x, y))
			return &this->map[index(x, y)];
		else
			return nullptr;
	}
	inline const T* atSafe(int x, int y) const {
		if (valid(x, y))
			return &this->map[index(x, y)];
		else
			return nullptr;
	}
	inline void set(const tx::Coord& pos, const T& val) {
		this->at(pos.x(), pos.y()) = val;
	}
	inline void set(int x, int y, const T& val) {
		this->at(x, y) = val;
	}
	inline bool valid(const tx::Coord& pos) const {
		return inRange_p(pos, dimension);
	}
	// inline bool valid(int x, int y) const {
	// 	return (x < this->Width && y < this->Height && x > -1 && y > -1);
	// }
	inline bool valid(int x, int y) const {
		return inRange_p(Coord{ x, y }, dimension);
	}
	inline int getWidth() const { return dimension.x(); }
	inline int getHeight() const { return dimension.y(); }
	inline int size() const { return this->map.size(); }
	//inline tx::Coord getCoord(const tx::vec2& in) const { return tx::Coord{ static_cast<int>((in.x() + 1.0f) / 2.0f * Width), static_cast<int>((in.y() + 1.0f) / 2.0f * Height) }; }
	inline int index(int x, int y) const { return y * dimension.x() + x; }
	inline int index(const tx::Coord& in) const { return index(in.x(), in.y()); }


	inline void reinit(int in_sideLen) {
		map.clear();
		this->map.assign(sq(in_sideLen), T{});
		dimension.set(in_sideLen, in_sideLen);
	}
	inline void reinit(int width, int height) {
		map.clear();
		this->map.assign(width * height, T{});
		dimension.set(width, height);
	}
	inline void reinit(const Coord& in_dimension) {
		map.clear();
		this->map.assign(in_dimension.x() * in_dimension.y(), T{});
		dimension = in_dimension;
	}

	inline std::vector<T>& data() { return map; }


	template <class Func>
	void foreach (Func&& func) {
		static_assert(
		    std::is_invocable_v<Func, T&> || std::is_invocable_v<Func, T&, tx::Coord&>,
		    "tx::GridSystem::foreach: invalid callback function");

		if constexpr (std::is_invocable_v<Func, T&, tx::Coord&>) {
			tx::Coord cur{ 0, 0 };
			for (; cur.y() < dimension.y(); cur.moveY(1)) {
				for (; cur.x() < dimension.x(); cur.moveX(1)) {
					func(at(cur), cur);
				}
				cur.setX(0);
			}
		} else {
			for (T& i : map) {
				func(i);
			}
		}
	}
	template <class Func>
	void foreach (const CoordMap& coords, Func && func) {
		static_assert(
		    std::is_invocable_v<Func, T&> || std::is_invocable_v<Func, T&, Coord&>,
		    "tx::GridSystem::foreach: invalid callback function");

		if constexpr (std::is_invocable_v<Func, T&, Coord&>) {
			for (const Coord& i : coords) {
				func(this->at(i), i);
			}
		} else {
			for (const Coord& i : coords) {
				func(this->at(i));
			}
		}
	}
	template <class Func>
	void foreachSafe(const CoordMap& coords, Func&& func) {
		static_assert(
		    std::is_invocable_v<Func, T&> || std::is_invocable_v<Func, T&, Coord&>,
		    "tx::GridSystem::foreach: invalid callback function");

		if constexpr (std::is_invocable_v<Func, T&, Coord&>) {
			for (const Coord& i : coords) {
				if (this->valid(i))
					func(this->at(i), i);
			}
		} else {
			for (const Coord& i : coords) {
				if (this->valid(i))
					func(this->at(i));
			}
		}
	}

private:
	std::vector<T> map;
	Coord dimension;
};
} // namespace tx
