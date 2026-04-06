// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXUtility
// File: type_traits.h

#pragma once
#include "impl/basic_utils.hpp"
#include <type_traits>

namespace tx {

// all enum should have underlying type of u32
template <class T>
    requires std::is_enum_v<T> || std::is_same_v<std::underlying_type_t<T>, u32>
inline constexpr u32 enumval(T in) noexcept {
	return static_cast<u32>(in);
}

template <class>
inline constexpr bool false_v = false;

// Concepts

// is instantiatin of
template <typename T, template <typename...> class Template>
struct is_instantiation_of : std::false_type {};
template <template <typename...> class Template, typename... Args>
struct is_instantiation_of<Template<Args...>, Template> : std::true_type {};
template <typename T, template <typename...> class Template>
concept InstantiationOf = is_instantiation_of<T, Template>::value;

template <class RetT, class Func, class... Args>
concept invocable_r = std::is_invocable_r_v<RetT, Func, Args...>;

enum class TypeEnum : u32 {
	Float,
	Int,
	U8,
	U32,
	Vec2,
	Coord
};

template <class T>
struct type_enum {
	static_assert(false_v<T>, "tx::type_enum: Unsupported type.");
};
template <>
struct type_enum<float> {
	static constexpr TypeEnum value = TypeEnum::Float;
};
template <>
struct type_enum<int> {
	static constexpr TypeEnum value = TypeEnum::Int;
};
template <>
struct type_enum<u8> {
	static constexpr TypeEnum value = TypeEnum::U8;
};
template <>
struct type_enum<u32> {
	static constexpr TypeEnum value = TypeEnum::U32;
};
template <>
struct type_enum<tx::vec2> {
	static constexpr TypeEnum value = TypeEnum::Vec2;
};
template <>
struct type_enum<tx::Coord> {
	static constexpr TypeEnum value = TypeEnum::Coord;
};

template <class T>
inline constexpr TypeEnum type_enum_v = type_enum<T>::value;


template <TypeEnum T>
struct enum_type;

template <>
struct enum_type<TypeEnum::Float> {
	using type = float;
};
template <>
struct enum_type<TypeEnum::Int> {
	using type = int;
};
template <>
struct enum_type<TypeEnum::U8> {
	using type = u8;
};
template <>
struct enum_type<TypeEnum::U32> {
	using type = u32;
};
template <>
struct enum_type<TypeEnum::Vec2> {
	using type = tx::vec2;
};
template <>
struct enum_type<TypeEnum::Coord> {
	using type = tx::Coord;
};

template <TypeEnum T>
using enum_type_t = typename enum_type<T>::type;

}; // namespace tx