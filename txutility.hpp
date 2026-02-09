// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// File: TXLib_Utility

#pragma once
#include "txlib.hpp"
#include "txgraphics.hpp"
#include "txmath.hpp"

namespace tx {

	// all bitmap should be sorted
	using Bitmap = vector<Coord>;
	void sortBitmap(Bitmap& map) {
		std::sort(map.begin(), map.end(), [](const Coord& a, const Coord& b) {
				if (a.y() == b.y()) {
					return a.x() < b.x();
				} else {
					return a.y() < b.y();
				}
			});
		auto it = std::unique(map.begin(), map.end());
		map.erase(it, map.end());
	}
	void clampBitmap(Bitmap& map, const Coord& bottomLeft, const Coord& topRight){
		map.erase(std::remove_if(map.begin(), map.end(), [&](const Coord& in) {
			return tx::inRange(in, bottomLeft, topRight);
		}));
	}

	// (NDC) Normalized Device Coordinates, also known as the [-1, 1] range, is which OpenGL used for it's coordinate system

	tx::vec2 worldToNDC(const tx::vec2& worldPos, float scale) {
		return worldPos * 2.0f / scale - 1.0f;
	}
	tx::vec2 NDCtoWorld(const tx::vec2 pos, float scale) {
		return (pos + 1.0f) / 2.0f * scale;
	}

	template<class T>
	class GridSystem {
	public:
		GridSystem() {}
		GridSystem(int in_SideLen) :
			Width(in_SideLen), Height(in_SideLen)
		{
			this->map.assign(tx::sq(in_SideLen), T{});
		}
		GridSystem(int in_width, int in_height) :
			Width(in_width), Height(in_height)
		{
			this->map.assign(in_width * in_height, T{});
		}


		void fill(const vector<tx::Coord>& coords, const T& val) {
			for (const tx::Coord& i : coords) {
				this->at(i) = val;
			}
		}
		void clear(const T& val = T{}) {
			std::fill(this->map.begin(), this->map.end(), val);
		}



		inline T& atIndex(int index) { return this->map[index]; }
		inline T& at(const tx::Coord& pos) {
			return this->at(pos.x(), pos.y());
		}
		inline T& at(int x, int y) {
			return this->map[index(x, y)];
		}
		inline T* atSafe(const tx::Coord& pos) {
			return this->atSafe(pos.x(), pos.y());
		}
		inline T* atSafe(int x, int y) {
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
			return valid(pos.x(), pos.y());
		}
		inline bool valid(int x, int y) const {
			return (x < this->Width && y < this->Height && x > -1 && y > -1);
		}
		inline int getWidth() const { return this->Width; }
		inline int getHeight() const { return this->Height; }
		inline int size() const { return this->map.size(); }
		inline int rowsize() const { return this->Width; }
		inline tx::Coord getCoord(const tx::vec2& in) const { return tx::Coord{ static_cast<int>((in.x() + 1.0f) / 2.0f * Width), static_cast<int>((in.y() + 1.0f) / 2.0f * Height) }; }
		inline int index(int x, int y) const { return y * this->Width + x; }
		inline int index(const tx::Coord& in) const { return index(in.x(), in.y()); }


		inline void reinit(int in_sideLen) {
			map.clear();
			this->map.assign(tx::sq(in_sideLen), T{});
			Width = Height = in_sideLen;
		}

		inline vector<T>& data() { return map; }

		
		template<class Func>
		void foreach(const Func& func){
			static_assert(
				std::is_invocable_v<Func, T&> || std::is_invocable_v<Func, T> || 
				std::is_invocable_v<Func, T&, tx::Coord&> || std::is_invocable_v<Func, T&, tx::Coord> ||
				std::is_invocable_v<Func, T , tx::Coord&> || std::is_invocable_v<Func, T , tx::Coord>,
				"tx::GridSystem::foreach: invalid callback function");
			
			if constexpr (std::is_invocable_v<Func, T&, tx::Coord&>) {
				tx::Coord cur{0, 0};
				for(; cur.y() < Height; cur.moveY(1)) {
					for(; cur.x() < Width; cur.moveX(1)){
						func(at(cur), cur);
					} cur.setX(0);
				}
			} else {
				for(T& i : map){
					func(i);
				}
			}
		}


	private:
		vector<T> map;
		int Width = 0, Height = 0;
	};

	class Rect {
	public:
		Rect(const vec2& in_pos, float in_w, float in_h) :
			m_bottomLeft(in_pos), m_width(in_w), m_height(in_h) {}
		Rect(const vec2& in_pos, const vec2& diagonalVec) :
			m_width(std::abs(diagonalVec.x())), m_height(std::abs(diagonalVec.y())),
			m_bottomLeft(vec2{
			diagonalVec.x() < 0.0f ? in_pos.x() + diagonalVec.x() : in_pos.x(),
			diagonalVec.y() < 0.0f ? in_pos.y() + diagonalVec.y() : in_pos.y()
				}) {}

		inline vec2 topRight()    const { return m_bottomLeft + vec2(m_width, m_height); }
		inline vec2 topLeft()     const { return m_bottomLeft + vec2(0.0f, m_height); }
		inline vec2 bottomRight() const { return m_bottomLeft + vec2(m_width, 0.0f); }
		inline vec2 bottomLeft()  const { return m_bottomLeft; }
		inline vec2 center()      const { return m_bottomLeft + vec2(m_width * 0.5f, m_height * 0.5f); }

		inline float width () const { return m_width; }
		inline float height() const { return m_height; }
		
	private:
		vec2 m_bottomLeft;
		float m_width, m_height;
	};
	inline Rect makeRange(const vec2& in_bottomLeft, const vec2& in_topRight) {
		return Rect{ in_bottomLeft, in_topRight - in_bottomLeft };
	}

	inline void drawRect(const Rect& rect) { drawRect(rect.topLeft(), rect.width(), rect.height()); }

	// inclusive-inclusive
	class DiscreteRect {
	public:
		DiscreteRect(const Coord& in_bottomLeft, int in_w, int in_h) :
			m_bottomLeft(in_bottomLeft), width(in_w), height(in_h) {}
		// inclusive-inclusive
		DiscreteRect(const Coord& start, const Coord& end) : // inclusive-inclusive
			width(std::abs(start.x() - end.x() + 1)), height(std::abs(start.y() - end.y() + 1)),
			m_bottomLeft(Coord{
			min(start.x(), end.x()),
			min(start.y(), end.y())
				}) {}

		Coord topRight()    const { return Coord{ m_bottomLeft.x() + width - 1, m_bottomLeft.y() + height - 1 }; }
		Coord topLeft()     const { return Coord{ m_bottomLeft.x(),             m_bottomLeft.y() + height - 1 }; }
		Coord bottomRight() const { return Coord{ m_bottomLeft.x() + width - 1, m_bottomLeft.y() }; }
		Coord bottomLeft()  const { return m_bottomLeft; }
		Coord center()      const { return Coord{ static_cast<int>(m_bottomLeft.x() + width * 0.5), static_cast<int>(m_bottomLeft.y() + height * 0.5) }; }

	private:
		Coord m_bottomLeft; // inclusive
		int width, height;
	};
	using DRect = DiscreteRect;








	// index start from 0 to sideLen - 1
	// using positive quadrant (math coordinate system)
	class PixelEngine {
	public:
		static void drawRow(int y, int x_start, int x_end, const tx::RGB& color, float GridSize, vec2 offset) {
			if (color == tx::Black) return;
			tx::glColorRGB(color);
			drawRow(y, x_start, x_end, GridSize, offset);
		}
		static void drawRow(int y, int x_start, int x_end, float GridSize, const vec2& offset) {
			float width = (x_end - x_start + 1) * GridSize;
			tx::vec2 topLeft(x_start * GridSize - 1.0f, y * GridSize - 1.0f + GridSize);
			tx::drawRect(topLeft + offset, width, GridSize);
		}
		template<class T, class Func>
		static void draw(
			GridSystem<T>& gs, 
			Func modifierCallback,
			const vec2& pos = BottomLeft, // bottom left of the map
			float mapSize = 2.0f) {
			using ReturnVal = std::invoke_result_t<Func, T>;
			static_assert(std::is_invocable_v<Func, T>,
				"tx::PixelEngine::draw(): modifierCallback provided must take in a parameter of T(type of tx::GridSystem provided). The provided callable don't match the requirement.");
			static_assert(std::is_same_v<std::decay_t<ReturnVal>, RGB>,
				"tx::PixelEngine::draw(): modifierCallback provided must return a tx::RGB object or a refernce to it. The provided callable don't match the requirement.");
			static_assert(std::is_convertible_v<decltype(std::declval<T>() != std::declval<T>()), bool>,
				"tx::PixelEngine::draw(): the type of GridSystem<T> must be comparable with operator!=.");

			//cout << "start\n";

			float gridSize = mapSize / gs.getWidth();
			vec2 offset = pos - BottomLeft;

			T previous = gs.at(0, 0);
			int previousX = 0;
			int index = 0;
			for (int y = 0; y < gs.getHeight(); ++y) {
				//cout << "row: " << y << "\n\n";
				previousX = 0;
				previous = gs.atIndex(index);
				for (int x = 0; x < gs.getWidth(); ++x) {
					//cout << "col: " << x;
					if (previous != gs.atIndex(index)) {
						// draw previous
						drawRow(y, previousX, x - 1, modifierCallback(previous), gridSize, offset);
						previous = gs.atIndex(index);
						previousX = x;
					}
					++index;
				}
				// draw last piece
				drawRow(y, previousX, gs.getWidth() - 1, modifierCallback(previous), gridSize, offset);
			}
		}
		static void drawBitmap(const Bitmap& map, float sideLen, const Coord& offset = CoordOrigin) {
			if (map.empty()) return;
			float GridSize = getGridSize(sideLen);

			//auto getCoord = [&](const Coord& in) { return in + offset; };
						
			int startX = map.front().x();
			int previousX = startX, previousY = map.front().y();
			auto finishRowSeg = [&](const Coord& cur) {
				drawRow(previousY, startX, previousX, GridSize, BottomLeft);
				startX = cur.x();
				};

			for (const Coord& i : map) {
				if (previousY != i.y() || i.x() - previousX > 1) { // switch line or x gap
					finishRowSeg(i);
					previousY = i.y();
				} previousX = i.x();
			}
			// draw last segment
			drawRow(previousY, startX, previousX, GridSize, BottomLeft);
		}
		static void drawBoolmap(GridSystem<uint8_t>& gs) {
			draw(gs, getBWColor);
		}
		static void drawRGBmap(GridSystem<RGB>& gs, const vec2& pos = BottomLeft, float mapSize = 2.0f) {
			draw(gs, [](const RGB& in) -> const RGB& { return in; }, pos, mapSize);
		}
		static constexpr inline float getGridSize(int in_sideLen) { return 2.0f / in_sideLen; }

	};


	

	class GridCircle {
	public:
		GridCircle(float in_radius) {
			float rsq = tx::sq(in_radius);
			this->radius = std::ceil(in_radius - 0.5f);
			int sectorRange = this->radius - 1;
			float height = 0.5f;
			for (int i = 0; i < sectorRange; i++) {
				//sector.push_back(std::cos(std::asinf(height / in_radius)) * in_radius)
				sector.push_back(std::ceil(std::sqrtf(rsq - tx::sq(height)) - 0.5f));
				height += 1.0f;
			}
			sector.push_back(std::ceil(std::sqrtf(rsq - tx::sq(height)) - 0.5f));
		}

		void getBitMap(const tx::Coord& center, vector<tx::Coord>& buffer) {
			buffer.reserve(getGridAmount());
			buffer.clear();
			buffer.push_back(center);
			for (int j = 0; j < 2; j++) {
				tx::Coord temp = center;
				for (int i = 0; i < this->radius; i++) {
					temp += _4wayIncrement[j];
					buffer.push_back(temp);
				}
			}

			for (int row = 0; row < this->sector.size(); row++) {
				int rowOffset = this->sector[row];
				for (int i = -rowOffset; i <= rowOffset; i++) {
					buffer.push_back(center + tx::Coord(i, row + 1));
					buffer.push_back(center + tx::Coord(i, -(row + 1)));
				}
			}
			sortBitmap(buffer);
		}

		template<class T>
		void applyToGridSys(const tx::Coord& center, GridSystem<T>& gs, const T& val) {
			gs.set(center, val);
			for (int j = 0; j < 2; j++) {
				tx::Coord temp = center;
				for (int i = 0; i < this->radius; i++) {
					temp += _4wayIncrement[j];
					gs.set(temp, val);
				}
			}

			for (int row = 0; row < this->sector.size(); row++) {
				int rowOffset = this->sector[row];
				for (int i = -rowOffset; i <= rowOffset; i++) {
					gs.set(center + tx::Coord(i,  row + 1), val);
					gs.set(center + tx::Coord(i, -row - 1), val);
				}
			}
		}

		int getGridAmount() {
			return std::ceil(tx::sq(this->radius) * PI);
			//return 4 * (tx::sq(this->radius) - this->radius) + 1;
		}

	private:
		vector<int> sector; // the x offset for each row from the center
		int radius;



	};

	class GridLine {
		// terminology:
		// OFB = Out of Bound
	public:
		GridLine() {}
		GridLine(const tx::vec2& in_start, const tx::vec2& in_end, int in_width, int in_height) :
			width(in_width), height(in_height), topRight(in_width, in_height), rangeEnd(in_width, in_height), line(tx::makeLineSegment(in_start, in_end))
		{
			init_impl(in_start, in_end);


		}

		inline const vector<tx::Coord>& getBitMap() const { return this->passedGrids; }

		inline void reinit(const tx::vec2& in_start, const tx::vec2& in_end, int in_width, int in_height) {
			width = in_width;
			height = in_height;
			rangeEnd = tx::Coord{ in_width, in_height };
			topRight = tx::toVec2(rangeEnd);
			line = tx::makeLineSegment(in_start, in_end);

			passedGrids.clear();

			init_impl(in_start, in_end);
		}

	private:
		vector<tx::Coord> passedGrids;
		int width, height;
		tx::vec2 topRight;
		tx::DLineSeg line;
		tx::Coord current, end;
		tx::Coord direction;
		tx::Coord directionIncrement;
		tx::Coord rangeEnd;
		float currentT, endT;
		bool valid = 1;
		int maxMarchAmount;

		void init_impl(const tx::vec2& in_start, const tx::vec2& in_end) {
			passedGrids.reserve(static_cast<int>(line.dVec().length() * 2));
			direction = tx::find8wayDir(line.dVec());
			directionIncrement = [&]() -> tx::Coord {
				return tx::Coord{
					direction.x() > 0 ? 1 : 0,
					direction.y() > 0 ? 1 : 0
				};
			}();
			findStartEnd_impl(in_start, in_end);
			if (current == end) {
				if (tx::inRange(current, tx::CoordOrigin, rangeEnd)) passedGrids.push_back(current);
			}
			else if (current.x() == end.x() || current.y() == end.y()) {
				//cout << "march straight\n";
				marchStraight_impl();
			}
			else {
				//cout << "march grids\n";
				marchGrids_impl();
			} tx::sortBitmap(passedGrids);
		}

		void findStartEnd_impl(const tx::vec2& in_start, const tx::vec2 in_end) {
			tx::vec2 startEndT = liang_findInRangeStartEndT(in_start, in_end, tx::Origin, topRight);
			this->current = tx::toCoord(line.findPoint(startEndT.x()) + tx::toVec2(direction) * tx::epsilon * 3.0f); // 3.0f is just a magic number to make the epsilon bigger
			this->end = tx::toCoord(line.findPoint(startEndT.y()));
			currentT = startEndT.x();
			endT = startEndT.y();
			direction = tx::find8wayDir(end - current);
			//passedGrids.push_back(current);
			//passedGrids.push_back(end);
		}
		void marchGrids_impl() {
			float t = currentT;
			while (!tx::inRange(current, tx::CoordOrigin, rangeEnd)) t = findNextCoord_impl();
			while (t <= endT) {
				passedGrids.push_back(current);
				t = findNextCoord_impl();
			} passedGrids.push_back(current);
			while (!tx::inRange(passedGrids.back(), tx::CoordOrigin, rangeEnd)) passedGrids.pop_back();


		}
		float findNextCoord_impl() {
			float nextxt = line.findTviaX(static_cast<float>(current.x() + directionIncrement.x()));
			float nextyt = line.findTviaY(static_cast<float>(current.y() + directionIncrement.y()));
			if (nextyt > nextxt) {
				this->current.moveX(direction.x());
				return nextxt;
			}
			else if (nextyt < nextxt) {
				this->current.moveY(direction.y());
				return nextyt;
			}
			else {
				this->current += direction;
				return nextxt;
			}
			//current = tx::toCoord(line.findPoint(tx::min(nextyt, nextxt)));
		}
		void marchStraight_impl() {
			while (!tx::inRange(current, tx::CoordOrigin, rangeEnd)) current += direction;
			//cout << "current: " << current << " end: " << end << " direction: " << direction << '\n';
			while (current != end) {
				passedGrids.push_back(current);
				current += direction;
			} passedGrids.push_back(current);
			while (!tx::inRange(passedGrids.back(), tx::CoordOrigin, rangeEnd)) passedGrids.pop_back();
		}


		bool liang_solveOneConstraint(float p, float q, float& tStart, float& tEnd) {
			if (p == 0.0f) return q >= 0.0f;

			float tMeet = q / p;

			if (p > 0.0f) {
				if (tMeet < tStart) return false;
				if (tMeet < tEnd) tEnd = tMeet;
			}
			else { // p < 0.0f
				if (tMeet > tEnd) return false;
				if (tMeet > tStart) tStart = tMeet;
			} return true;
		}
		tx::vec2 liang_findInRangeStartEndT(const tx::vec2& start, const tx::vec2 end, const tx::vec2& rangeBottomLeft, const tx::vec2& rangeTopRight) {
			tx::vec2 dVec = end - start;
			float dx = dVec.x();
			float dy = dVec.y();

			const tx::vec2& min = rangeBottomLeft;
			const tx::vec2& max = rangeTopRight;

			float tStart = 0.0f;
			float tEnd = 1.0f;

			// order don't matter
			if (!liang_solveOneConstraint(-dx, start.x() - min.x(), tStart, tEnd)) { return tx::InvalidVec; } // left
			if (!liang_solveOneConstraint(-dy, start.y() - min.y(), tStart, tEnd)) { return tx::InvalidVec; } // bottom
			if (!liang_solveOneConstraint(dx, max.x() - start.x(), tStart, tEnd)) { return tx::InvalidVec; } // right
			if (!liang_solveOneConstraint(dy, max.y() - start.y(), tStart, tEnd)) { return tx::InvalidVec; } // top

			return tx::vec2{ tStart, tEnd };
		}
	};

	//Coord randBoundaryCoord(int start, int end) { // inclusive, exclusive
	//	static std::random_device rrde;
	//	static std::mt19937 rde(rrde());
	//	static std::uniform_int_distribution<int> dist_bool(0, 1);
	//	end -= 1;
	//	if (start > end) std::swap(start, end);
	//	std::uniform_int_distribution<int> dist(start, end);
	//	bool vertical = dist_bool(rde); // x = start or end
	//	int x, y;
	//	(vertical ? x : y) = dist_bool(rde) ? start : end;
	//	(vertical ? y : x) = dist(rde);
	//	return tx::Coord{ x, y };
	//}
	Coord randBoundaryCoord(int xmin, int xmax, int ymin, int ymax) { // inclusive, inclusive
		static std::random_device rrde;
		static std::mt19937 rde(rrde());
		static std::uniform_int_distribution<int> dist_bool(0, 1);
		if (xmin > xmax) std::swap(xmin, xmax);
		if (ymin > ymax) std::swap(ymin, ymax);
		bool vertical = dist_bool(rde); // x = start or end
		int x, y;
		/*(vertical ? x : y) = dist_bool(rde) ? xmin : xmax;
		(vertical ? y : x) = std::uniform_int_distribution<int>(vertical ? ymin : xmin, vertical ? ymax : xmax)(rde);*/ // "smart" code right here. nobody could read this
		if (vertical) {
			x = dist_bool(rde) ? xmin : xmax;
			std::uniform_int_distribution<int> yDist(ymin, ymax);
			y = yDist(rde);
		} else {
			y = dist_bool(rde) ? ymin : ymax;
			std::uniform_int_distribution<int> xDist(xmin, xmax);
			x = xDist(rde);
		} return tx::Coord{ x, y };
	}
	inline Coord randBoundaryCoord(int start, int end) {
		return randBoundaryCoord(start, end, start, end);
	}


















}