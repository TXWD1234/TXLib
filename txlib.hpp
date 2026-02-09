// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// File: TXLib_General

#pragma once
#include "header.h"

using std::cout;
using std::cin;
using std::endl;
using std::vector;
using std::string;

namespace std {
	namespace fs = filesystem;
}


namespace tx {

	constexpr float epsilon = 1e-6f;

	// vec2 ***********************************************************************************************************************
	
	// 2 direction vector
	class vec2;
	class Coord;
	inline float dot(const vec2& in1, const vec2& in2);
	class vec2 {
	public:
		constexpr vec2(float in_x, float in_y) :
			m_x(in_x), m_y(in_y) {

		}
		constexpr vec2() : m_x(0.0f), m_y(0.0f) {}
		//explicit constexpr vec2(const Coord& in) : m_x(in.x()), m_y(in.y()) {}

		constexpr inline void set(float in_x, float in_y) { this->m_x = in_x; this->m_y = in_y; }
		constexpr inline void setX(float in_x) { this->m_x = in_x; }
		constexpr inline void setY(float in_y) { this->m_y = in_y; }
		constexpr inline float getX() const { return this->m_x; }
		constexpr inline float x() const { return this->m_x; }
		constexpr inline float getY() const { return this->m_y; }
		constexpr inline float y() const { return this->m_y; }

		constexpr inline vec2 operator+(const vec2& other) const { return vec2(this->m_x + other.m_x, this->m_y + other.m_y); }
		constexpr inline vec2 operator+(float other) const { return vec2(this->m_x + other, this->m_y + other); }
		constexpr inline vec2 operator-(const vec2& other) const { return vec2(this->m_x - other.m_x, this->m_y - other.m_y); }
		constexpr inline vec2 operator-(float other) const { return vec2(this->m_x - other, this->m_y - other); }
		//constexpr inline vec2 operator*(const vec2& other) const { return vec2(this->m_x * other.m_x, this->m_y * other.m_y); }
		constexpr inline vec2 operator*(float coef) const { return vec2(this->m_x * coef, this->m_y * coef); }
		constexpr inline vec2 operator/(float coef) const { return vec2(this->m_x / coef, this->m_y / coef); }
		constexpr inline vec2& operator+=(const vec2& other) { this->m_x += other.m_x; this->m_y += other.m_y; return *this; }
		constexpr inline vec2& operator+=(float other) { this->m_x += other; this->m_y += other; return *this; }
		constexpr inline vec2& operator-=(const vec2& other) { this->m_x -= other.m_x; this->m_y -= other.m_y; return *this; }
		constexpr inline vec2& operator-=(float other) { this->m_x -= other; this->m_y -= other; return *this; }
		//constexpr inline vec2& operator*=(const vec2& other) { this->m_x *= other.m_x; this->m_y *= other.m_y; return *this; }
		constexpr inline vec2& operator*=(float coef) { this->m_x *= coef; this->m_y *= coef; return *this; }
		//vec2 operator/(const vec2& other) { return vec2(this->m_x / other.m_x, this->m_y / other.m_y); }

		//constexpr inline vec2& operator=(const vec2& other) { this->m_x = other.m_x; this->m_y = other.m_y; return *this; }
		//constexpr inline vec2& operator=(float other) { this->m_x = other; this->m_y = other; return *this; }
		inline bool operator==(const vec2& other) const { return std::fabs(this->m_x - other.m_x) <= epsilon && std::fabs(this->m_y - other.m_y) <= epsilon; }
		inline bool operator!=(const vec2& other) const { return !(this->operator==(other)); }
		constexpr inline bool operator<(const vec2& other) const { return this->m_x * this->m_x + this->m_y * this->m_y < other.m_x * other.m_x + other.m_y * other.m_y; }
		constexpr inline bool operator>(const vec2& other) const { return this->m_x * this->m_x + this->m_y * this->m_y > other.m_x * other.m_x + other.m_y * other.m_y; }

		inline float length() const {
			return std::sqrtf(dot(*this, *this));
		}

		constexpr inline vec2 offset(float x, float y) const { return this->operator+(vec2(x, y)); }
		constexpr inline vec2 offsetX(float in) const { return vec2(this->m_x + in, this->m_y); }
		constexpr inline vec2 offsetY(float in) const { return vec2(this->m_x, this->m_y + in); }


	private:
		float m_x, m_y;
	};
	constexpr inline vec2 operator*(float coef, vec2 vec) { return vec2(vec.getX() * coef, vec.getY() * coef); }
	constexpr inline vec2 operator/(float coef, vec2 vec) { return vec2(vec.getX() / coef, vec.getY() / coef); }

	// Coord **********************************************************************************************************************

	// 2d integer coordinate
	class Coord {
	public:
		constexpr Coord(int in_x, int in_y) :
			m_x(in_x), m_y(in_y) {
		}
		constexpr Coord(int in_pos) :
			m_x(in_pos), m_y(in_pos) {
		}
		/*explicit Coord(const vec2& in_pos) :
			m_x(std::floor(in_pos.x())), m_y(std::floor(in_pos.y())) {
		}*/
		constexpr Coord() : m_x(0), m_y(0) {}
		constexpr inline void set(int in_x, int in_y) { this->m_x = in_x; this->m_y = in_y; }
		constexpr inline void setX(int in_x) { this->m_x = in_x; }
		constexpr inline void setY(int in_y) { this->m_y = in_y; }
		constexpr inline int getX() const { return this->m_x; }
		constexpr inline int x() const { return this->m_x; }
		constexpr inline int getY() const { return this->m_y; }
		constexpr inline int y() const { return this->m_y; }

		constexpr inline Coord operator+(const Coord& other) const { return Coord(this->m_x + other.m_x, this->m_y + other.m_y); }
		constexpr inline Coord operator-(const Coord& other) const { return Coord(this->m_x - other.m_x, this->m_y - other.m_y); }
		constexpr inline Coord operator+=(const Coord& other) { this->m_x += other.m_x; this->m_y += other.m_y; return *this; }
		constexpr inline Coord operator-=(const Coord& other) { this->m_x -= other.m_x; this->m_y -= other.m_y; return *this; }


		constexpr inline Coord operator=(const Coord& other) { this->m_x = other.m_x; this->m_y = other.m_y; return *this; }
		constexpr inline bool operator==(const Coord& other) const { return this->m_x == other.m_x && this->m_y == other.m_y; }
		constexpr inline bool operator!=(const Coord& other) const { return !(this->m_x == other.m_x && this->m_y == other.m_y); }
		constexpr inline bool operator<(const Coord& other) const { return this->m_x * this->m_x + this->m_y * this->m_y < other.m_x * other.m_x + other.m_y * other.m_y; }
		constexpr inline bool operator>(const Coord& other) const { return this->m_x * this->m_x + this->m_y * this->m_y > other.m_x * other.m_x + other.m_y * other.m_y; }

		constexpr inline vec2 operator*(float in) const { return vec2{ m_x * in, m_y * in }; }

		constexpr inline bool valid(int edge) {
			return m_x >= 0 && m_y >= 0 && m_x < edge && m_y < edge;
		}

		constexpr inline Coord offset(int in_x, int in_y) const { return Coord{ this->m_x + in_x, this->m_y + in_y }; }
		constexpr inline Coord offsetX(int in) const { return Coord(this->m_x + in, this->m_y); }
		constexpr inline Coord offsetY(int in) const { return Coord(this->m_x, this->m_y + in); }
		constexpr inline void move(int in_x, int in_y) { this->m_x += in_x; this->m_y += in_y; }
		constexpr inline void moveX(int in) { this->m_x += in; }
		constexpr inline void moveY(int in) { this->m_y += in; }

		

	private:
		int m_x, m_y;
	};

	inline Coord toCoord(const vec2&  in) { return Coord{ static_cast<int>(std::floor(in.x())), static_cast<int>(std::floor(in.y())) }; }
	inline vec2  toVec2 (const Coord& in) { return vec2 { static_cast<float>(in.x()),           static_cast<float>(in.y()) }; }

	inline std::ostream& operator<<(std::ostream& in_cout, const vec2&  in_vec2 ) { in_cout << "( " << in_vec2 .x() << ", " << in_vec2 .y() << " )"; return in_cout; }
	inline std::ostream& operator<<(std::ostream& in_cout, const Coord& in_coord) { in_cout << "( " << in_coord.x() << ", " << in_coord.y() << " )"; return in_cout; }


	// Math ***********************************************************************************************************************
	constexpr const vec2 IHat(1.0f, 0.0f);
	constexpr const vec2 JHat(0.0f, 1.0f);
	constexpr const vec2 Origin(0.0f, 0.0f);
	constexpr const vec2 InvalidVec(NAN, NAN);

	constexpr float PI = 3.1415926f;
	constexpr float ONE_DEGREE = 0.017453292f;

	constexpr Coord _8wayIncrement[] = {
		{  1,  0 },
		{ -1,  0 },
		{  0,  1 },
		{  0, -1 },
		{  1,  1 },
		{ -1,  1 },
		{ -1, -1 },
		{  1, -1 }
	};
	constexpr Coord _4wayIncrement[] = {
		{  1,  0 }, // right
		{ -1,  0 }, // left
		{  0,  1 }, // top
		{  0, -1 }  // bottom
	};
	constexpr int _2wayIncrement[] = {
		1, -1
	};
	constexpr Coord CoordOrigin{ 0, 0 };
	
	constexpr vec2 TopRight    = {  1.0,  1.0 };
	constexpr vec2 TopLeft     = { -1.0,  1.0 };
	constexpr vec2 BottomLeft  = { -1.0, -1.0 };
	constexpr vec2 BottomRight = {  1.0, -1.0 };
	
	template<class T>
	constexpr inline T sign(T num) { return (num == 0 ? 1 : num / std::abs(num)); }
	inline float dot(const vec2& in1, const vec2& in2) { return (in1.getX() * in2.getX() + in1.getY() * in2.getY()); }
	constexpr inline float sq(float in) { return in * in; }
	constexpr inline double sq(double in) { return in * in; }
	constexpr inline int sq(int in) { return in * in; }
	inline float hypotsq(const vec2& in) { return dot(in, in); }
	inline vec2 unify(const vec2& in, float* len = nullptr) {
		float scalar = std::sqrtf(dot(in, in));
		if (scalar == 0.0f) return vec2(0, 0);
		if (len != nullptr) *len = scalar;
		return vec2(in.getX() / scalar, in.getY() / scalar);
	}
	constexpr inline vec2 unify(const vec2& in, float dist) {
		if (dist == 0.0f) return vec2(0, 0);
		return vec2(in.getX() / dist, in.getY() / dist);
	}
	constexpr inline vec2 reverse(const vec2& in) { return in * -1.0f; }
	constexpr inline vec2 operator-(const vec2& in) { return in * -1.0f; }
	constexpr inline vec2 mirror(const vec2& in) { return vec2(in.y(), in.x()); }
#undef min
#undef max
	template<class T>
	constexpr inline T min(T a, T b) { return a < b ? a : b; }
	template<class T>
	constexpr inline T max(T a, T b) { return a > b ? a : b; }
	template<class T>
	constexpr inline T min(const vector<T>& vec) {
		if (!vec.size()) { return T{}; }
		T minv = vec[0];
		for (const T& i : vec) {
			minv = min(minv, i); // 1
			//if (i < minv) minv = i; // 2
		} return minv;
	}
	template<class T>
	constexpr inline T max(const vector<T>& vec) {
		if (!vec.size()) { return T{}; }
		T maxv = vec[0];
		for (const T& i : vec) {
			maxv = max(maxv, i);
		} return maxv;
	}
	template<class T>
	inline T sum(const vector<T>& vec) {
		if (!vec.size()) return T{};
		T sum{};
		for (int i = 0; i < vec.size(); i++) {
			sum += vec[i];
		} return sum;
	}
	template<class T>
	inline double average(const vector<T>& vec) {
		if (!vec.size()) return T{};
		return sum(vec) / (double)vec.size();
	}
	inline vec2 orbitalPos(float degree) { return vec2{ std::cosf(degree), std::sinf(degree) }; }
	inline vec2 abs(const vec2& in) { return vec2{ std::fabs(in.x()), std::fabs(in.y()) }; }
	inline bool isValid(double in) { return !std::isnan(in) && !std::isinf(in); }
	inline bool isValid(const vec2& in) { return isValid(in.x()) && isValid(in.y()); }
	constexpr inline vec2 leftPerp (const vec2& in) { return vec2{ -in.y(), in.x() }; }
	constexpr inline vec2 rightPerp(const vec2& in) { return vec2{ in.y(), -in.x() }; }
	template<class T>
	constexpr inline bool inRange(T val, T min, T max) { // inclusive
		return val >= min && val <= max;
	}
	// inclusive, exclusive
	constexpr inline bool inRange(const Coord& pos, const Coord& bottomLeft, const Coord& topRight) { // inclusive, exclusive
		return pos.x() >= bottomLeft.x() && pos.x() < topRight.x() && pos.y() >= bottomLeft.y() && pos.y() < topRight.y();
	}
	constexpr inline bool inRange(const vec2& pos, const vec2& bottomLeft, const vec2& topRight) {
		return pos.x() >= bottomLeft.x() && pos.x() <= topRight.x() && pos.y() >= bottomLeft.y() && pos.y() <= topRight.y();
	}
	constexpr inline int makeOdd(int in) { // by ++
		return (in % 2 ? in : in + 1);
	}
	constexpr inline Coord center(int sideLen) {
		return Coord{ static_cast<int>(sideLen * 0.5) };
	}
	template<class T>
	constexpr inline T clamp(const T& val, const T& in_min, const T& in_max) {
		return max(min(val, in_max), in_min);
	}
	inline Coord find8wayDir(const vec2& vec) {
		return Coord{
			vec.x() == 0.0f ? 0 : static_cast<int>(sign(vec.x())),
			vec.y() == 0.0f ? 0 : static_cast<int>(sign(vec.y()))
		};
	}
	inline Coord find8wayDir(const Coord& vec) {
		return Coord{
			vec.x() == 0 ? 0 : sign(vec.x()),
			vec.y() == 0 ? 0 : sign(vec.y())
		};
	}
	





	

	// Filesystem Functions **********************************************************************************************************
	// fp stands for filePath

	// std::filesystem::path getExePath() {
	// 	char buffer[MAX_PATH];
	// 	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	// 	return std::filesystem::path(buffer);
	// }
	// std::filesystem::path filePathInExeDir(const string& filename) {
	// 	return getExePath().parent_path() / filename;
	// }
	void readWholeFile(const std::filesystem::path& filePath, string& str) {
		size_t fileSize = std::filesystem::file_size(filePath);

		str = string(fileSize, '\0');
		std::ifstream ifs(filePath, std::ios::binary);
		if (!ifs) { throw std::runtime_error("Failed to open file"); }
		ifs.read(str.data(), fileSize);
		if (ifs.gcount() != fileSize) { throw std::runtime_error("Failed to read entire file"); }
	}



	// Log ************************************************************************************************************************
	class Log {
	public:
		template<class T>
		Log& operator<<(const T in) {
			if (!Log::isActive) return *this;
			this->oss << in;
			return *this;
		}
		Log& operator<<(void(*func)(Log* obj)) {
			if (!Log::isActive) return *this;
			func(this);
			return *this;
		}
		static void print(Log* obj) {
			std::lock_guard<std::mutex> lock(Log::mutex);
			cout << obj->oss.str() << Color::Reset << '\n';
			obj->oss.str("");
		}
		class Color {
		public:
			static constexpr const char* Reset = "\033[0m";
			static constexpr const char* Bold = "\033[1m";
			static constexpr const char* UnderLine = "\033[4m";

			static constexpr const char* Black = "\033[30m";
			static constexpr const char* Red = "\033[31m";
			static constexpr const char* Green = "\033[32m";
			static constexpr const char* Yellow = "\033[33m";
			static constexpr const char* Blue = "\033[34m";
			static constexpr const char* Magenta = "\033[35m";
			static constexpr const char* Cyan = "\033[36m";
			static constexpr const char* White = "\033[37m";

			static constexpr const char* BrightBlack = "\033[90m";
			static constexpr const char* BrightRed = "\033[91m";
			static constexpr const char* BrightGreen = "\033[92m";
			static constexpr const char* BrightYellow = "\033[93m";
			static constexpr const char* BrightBlue = "\033[94m";
			static constexpr const char* BrightMagenta = "\033[95m";
			static constexpr const char* BrightCyan = "\033[96m";
			static constexpr const char* BrightWhite = "\033[97m";
		};
	private:
		std::ostringstream oss;
		static std::mutex mutex;
		static bool isActive;
	};
	std::mutex Log::mutex;
	bool Log::isActive = 0;

	// Color and Rainbow **********************************************************************************************************
#undef RGB
	class RGB {
	public:
		constexpr RGB(float in_r, float in_g, float in_b) :
			m_r(in_r), m_g(in_g), m_b(in_b)
		{}
		constexpr RGB() :
			m_r{}, m_g{}, m_b{}
		{}

		constexpr inline const float r() const { return this->m_r; }
		constexpr inline const float g() const { return this->m_g; }
		constexpr inline const float b() const { return this->m_b; }

		inline bool operator==(const tx::RGB& other) const {
			return this->m_r == other.m_r && this->m_g == other.m_g && this->m_b == other.m_b;
		}
		inline bool operator!=(const tx::RGB& other) const { return !this->operator==(other); }

		inline void normalize() { 
			m_r /= 255.0f;
			m_g /= 255.0f;
			m_b /= 255.0f;
		}
		inline RGB normalized() const {
			return RGB{
				m_r / 255.0f,
				m_g / 255.0f,
				m_b / 255.0f
			};
		}
		inline void transpose() {
			std::swap(m_r, m_b);
		}
		inline RGB transposed() const {
			return RGB{m_b, m_g, m_r};
		}
	private:
		float m_r, m_g, m_b;
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

	constexpr RGB MikuColor      = RGB(0, 210, 255);
	constexpr RGB MikuColorBegin = RGB(0, 230, 255);
	constexpr RGB MikuColorEnd   = RGB(0, 255, 200);

	inline std::ostream& operator<<(std::ostream& in_cout, const RGB&  in_rgb ) { in_cout << "( " << in_rgb.r() << ", " << in_rgb.g() << ", " << in_rgb.b() << " )"; return in_cout; }



	class Rainbow {
	public:
		Rainbow(int range) { // maybe more rainbow generating algorithms?
			float increment = PI / range,
				third = PI / 3;
			for (int i = 0; i < range; i++) {
				m_rainbow.push_back(RGB(
					std::abs(std::sinf(increment * i)),
					std::abs(std::sinf(increment * i + third)),
					std::abs(std::sinf(increment * i + third * 2))
				));
			}
		}

		inline const RGB& operator[](int index) const {
			return m_rainbow[index];
		}
		inline const RGB& getNextColor() {
			index++;
			if (index == this->m_rainbow.size()) {
				index = 0;
			} return m_rainbow[index];
		}
	private:
		vector<RGB> m_rainbow;
		int index = -1;
	};

	// Local Time (inherited from txtime.h) ***********************************************************************************
	// create time strings with certain format
	namespace Time {

		std::tm getTime() {
			auto now = std::chrono::system_clock::now(); // ��ǰϵͳʱ���
			std::time_t time = std::chrono::system_clock::to_time_t(now); // ת�� C ���ʱ��
			std::tm localnow;
			localtime_r(&time, &localnow);
			return localnow;
		}
		string now(const char* format) {
			std::tm localnow = getTime();
			std::ostringstream oss;
			oss << std::put_time(&localnow, format);
			return oss.str();
		}

		namespace Format {
			constexpr const char* Date = "%Y/%m/%d";
			constexpr const char* Time = "%H:%M:%S";
			constexpr const char* DayTime = "%Y/%m/%d %H:%M:%S";
		}

		class Timer {
		public:
			Timer() : start(std::chrono::steady_clock::now()) {}

			template<class T = std::chrono::duration<double, std::milli>>
			inline auto duration() {
				auto end = std::chrono::steady_clock::now();
				auto duration = std::chrono::duration_cast<T>(end - start);
				return duration.count();
			}
			inline size_t durationL() {
				auto end = std::chrono::steady_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
				return duration.count();
			}
			inline void reset() {
				this->start = std::chrono::steady_clock::now();
			}
		private:
			std::chrono::steady_clock::time_point start;
		};





	}
	namespace TimeFormat = Time::Format;
	namespace TimeF = TimeFormat;



}



//// Enum Utility Overloads *****************************************************************************************************
//template<class Enum, class = std::enable_if_t<std::is_enum_v<Enum>>>
//inline Enum operator|(Enum a, Enum b) {
//	using T = std::underlying_type_t<Enum>;
//	return static_cast<Enum>(static_cast<T>(a) | static_cast<T>(b));
//}
//template<class Enum, class = std::enable_if_t<std::is_enum_v<Enum>>>
//inline Enum& operator|=(Enum& a, Enum b) {
//	using T = std::underlying_type_t<Enum>;
//	return (a = static_cast<Enum>(static_cast<T>(a) | static_cast<T>(b)));
//}
//template<class Enum, class = std::enable_if_t<std::is_enum_v<Enum>>>
//inline bool operator&(Enum a, Enum b) {
//	using T = std::underlying_type_t<Enum>;
//	return (static_cast<T>(a) & static_cast<T>(b));
//}