// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXUtility

#pragma once
#include "impl/basic_utils.hpp"
#include "tx/type_traits.hpp"

#include <concepts>

namespace tx::bit {
template <class T>
    requires std::integral<T> || std::is_enum_v<T>
void combine(T& a, T b) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		a = static_cast<T>(
		    static_cast<U>(a) | static_cast<U>(b));
	} else
		a |= b;
}
template <class T>
    requires std::integral<T> || std::is_enum_v<T>
T combined(T a, T b) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		return static_cast<T>(
		    static_cast<U>(a) | static_cast<U>(b));
	} else
		return a | b;
}

template <class T>
    requires std::integral<T> || std::is_enum_v<T>
void erase(T& a, T b) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		a = static_cast<T>(
		    static_cast<U>(a) & (~static_cast<U>(b)));
	} else {
		a &= (~b);
	}
}
template <class T>
    requires std::integral<T> || std::is_enum_v<T>
T erased(T a, T b) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		return static_cast<T>(
		    static_cast<U>(a) & (~static_cast<U>(b)));
	} else {
		return a & (~b);
	}
}

template <class T>
    requires std::integral<T> || std::is_enum_v<T>
bool contains(T a, T b) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		return static_cast<U>(a) & static_cast<U>(b) == static_cast<U>(b);
	} else {
		return (a & b) == b;
	}
}
} // namespace tx::bit