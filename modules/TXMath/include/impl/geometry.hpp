// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXMath

#pragma once
#include "impl/numeric_utils.hpp"
#include <cmath>
#include <iostream>

namespace tx {

// ################ Vector Implementation ################

template <size_t D, class T = f32>
class Vec {
	static_assert(D >= 1, "tx::Vec: D must be >= 1");

public:
	template <std::convertible_to<T>... Ts>
	    requires(sizeof...(Ts) == D)
	constexpr Vec(Ts... scalars) : data{ static_cast<T>(scalars)... } {}
	constexpr Vec(T s) { data.fill(s); };
	constexpr Vec() = default;

public:
	template <class Self>
	[[nodiscard]] constexpr auto&
	operator[](this Self&& self, size_t index) {
		// DevNote: add assert
		return self.data[index];
	}

	auto operator<=>(const Vec&) const = default;

private:
	std::array<T, D> data{};
};

// vec2 specialization
template <class T>
class Vec<2, T> {
public:
	constexpr Vec(T x_, T y_) : x(x_), y(y_) {}
	constexpr explicit Vec(T s) : x(s), y(s) {}
	constexpr Vec() = default;

	template <class Self>
	[[nodiscard]] constexpr auto&
	operator[](this Self&& self, size_t index) {
		// DevNote: add assert
		if (index == 0) return self.x;
		return self.y;
	}
	auto operator<=>(const Vec&) const = default;

	T x{}, y{};
};
// vec3 specialization
template <class T>
class Vec<3, T> {
public:
	constexpr Vec(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}
	constexpr explicit Vec(T s) : x(s), y(s), z(s) {}
	constexpr Vec() = default;

	template <class Self>
	[[nodiscard]] constexpr auto&
	operator[](this Self&& self, size_t index) {
		// DevNote: add assert
		if (index == 0) return self.x;
		if (index == 1) return self.y;
		return self.z;
	}
	auto operator<=>(const Vec&) const = default;

	T x{}, y{}, z{};
};

// DevNote: Move to foundation?
namespace impl {
template <size_t Num, std::invocable<size_t> Func>
constexpr void iterateIndexSequence(Func&& f) {
	for (size_t i = 0; i < Num; i++) f(i);
}
} // namespace impl

// anyone who looked at this must take a moment of silence for it
// template <size_t D, class T>
// Vec<D, T> operator+(Vec<D, T> a, Vec<D, T> b) {
// 	return [a, b]<size_t... Is>(std::index_sequence<Is...>) { return Vec<D, T>((a[Is] + b[Is])...); }(std::make_index_sequence<D>());
// }

template <size_t D, class T>
constexpr Vec<D, T>& operator+=(Vec<D, T>& a, const Vec<D, T>& b) {
	impl::iterateIndexSequence<D>([&](size_t i) { a[i] += b[i]; });
	return a;
}
template <size_t D, class T>
constexpr Vec<D, T>& operator-=(Vec<D, T>& a, const Vec<D, T>& b) {
	impl::iterateIndexSequence<D>([&](size_t i) { a[i] -= b[i]; });
	return a;
}
template <size_t D, class T>
constexpr Vec<D, T>& operator*=(Vec<D, T>& a, const Vec<D, T>& b) {
	impl::iterateIndexSequence<D>([&](size_t i) { a[i] *= b[i]; });
	return a;
}
template <size_t D, class T>
constexpr Vec<D, T>& operator*=(Vec<D, T>& a, T s) {
	impl::iterateIndexSequence<D>([&](size_t i) { a[i] *= s; });
	return a;
}
template <size_t D, class T>
constexpr Vec<D, T>& operator/=(Vec<D, T>& a, T s) {
	impl::iterateIndexSequence<D>([&](size_t i) { a[i] /= s; });
	return a;
}

template <size_t D, class T>
constexpr Vec<D, T> operator+(Vec<D, T> a, const Vec<D, T>& b) { return a += b; }
template <size_t D, class T>
constexpr Vec<D, T> operator-(Vec<D, T> a, const Vec<D, T>& b) { return a -= b; }
template <size_t D, class T>
constexpr Vec<D, T> operator*(Vec<D, T> a, const Vec<D, T>& b) { return a *= b; }
template <size_t D, class T>
constexpr Vec<D, T> operator*(Vec<D, T> a, T b) { return a *= b; }
template <size_t D, class T>
constexpr Vec<D, T> operator*(T b, Vec<D, T> a) { return a *= b; }
template <size_t D, class T>
constexpr Vec<D, T> operator/(Vec<D, T> a, T b) { return a /= b; }

template <size_t D, class T>
constexpr Vec<D, T> operator-(Vec<D, T> v) {
	impl::iterateIndexSequence<D>([&](size_t i) { v[i] = -v[i]; });
	return v;
}


using vec2 = Vec<2, f32>;
using ivec2 = Vec<2, i32>;
using Coord = ivec2;

inline std::ostream& operator<<(std::ostream& in_cout, const vec2& in_vec2) {
	in_cout << "( " << in_vec2.x << ", " << in_vec2.y << " )";
	return in_cout;
}
inline std::ostream& operator<<(std::ostream& in_cout, const Coord& in_coord) {
	in_cout << "( " << in_coord.x << ", " << in_coord.y << " )";
	return in_cout;
}

// ################ Constants ################
// clang-format off

constexpr const vec2 IHat(1.0f, 0.0f);
constexpr const vec2 JHat(0.0f, 1.0f);
constexpr const vec2 Origin(0.0f, 0.0f);
constexpr const vec2 InvalidVec(NAN, NAN);

constexpr vec2 TopRight = { 1.0, 1.0 };
constexpr vec2 TopLeft = { -1.0, 1.0 };
constexpr vec2 BottomLeft = { -1.0, -1.0 };
constexpr vec2 BottomRight = { 1.0, -1.0 };

constexpr vec2 directionVec[] = {
    {  1.0f,  0.0f },
    {  1.0f,  1.0f },
    {  0.0f,  1.0f },
    { -1.0f,  1.0f },
    { -1.0f,  0.0f },
    { -1.0f, -1.0f },
    {  0.0f, -1.0f },
    {  1.0f, -1.0f }
};

constexpr Coord _8wayIncrement[] = {
    {  1,  0 },
    {  1,  1 },
    {  0,  1 },
    { -1,  1 },
    { -1,  0 },
    { -1, -1 },
    {  0, -1 },
    {  1, -1 }
};
constexpr Coord _4wayIncrement[] = {
	{  1,  0 },
	{ -1,  0 },
	{  0,  1 },
	{  0, -1 }
};
constexpr int _2wayIncrement[] = {
	1, -1
};
constexpr Coord CoordOrigin{ 0, 0 };
// clang-format on
} // namespace tx


#include <functional>
// std::hash specialization so tx::Coord can be used in std::unordered_map
namespace std {
template <>
struct hash<tx::Coord> {
	size_t operator()(const tx::Coord& c) const noexcept {
		size_t h1 = std::hash<int>{}(c.x);
		size_t h2 = std::hash<int>{}(c.y);
		// Standard hash combining algorithm (from Boost)
		return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
	}
};
} // namespace std

#include "tx/type_traits.hpp"
namespace tx {
template <>
struct type_enum<vec2> {
	static constexpr TypeEnum value = TypeEnum::Vec2;
};
template <>
struct type_enum<Coord> {
	static constexpr TypeEnum value = TypeEnum::Coord;
};

template <>
struct enum_type<TypeEnum::Vec2> {
	using type = vec2;
};
template <>
struct enum_type<TypeEnum::Coord> {
	using type = Coord;
};
} // namespace tx