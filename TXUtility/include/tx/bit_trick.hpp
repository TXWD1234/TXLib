// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXUtility

#pragma once
#include "impl/basic_utils.hpp"
#include "tx/type_traits.hpp"

#include <concepts>

namespace tx::bit {

template <class T>
concept bit_eligible = std::integral<T> || std::is_enum_v<T>;

template <bit_eligible T>
void combine(T& a, T b) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		a = static_cast<T>(
		    static_cast<U>(a) | static_cast<U>(b));
	} else
		a |= b;
}
template <bit_eligible T>
T combined(T a, T b) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		return static_cast<T>(
		    static_cast<U>(a) | static_cast<U>(b));
	} else
		return a | b;
}

template <bit_eligible T>
void erase(T& a, T b) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		a = static_cast<T>(
		    static_cast<U>(a) & (~static_cast<U>(b)));
	} else {
		a &= (~b);
	}
}
template <bit_eligible T>
T erased(T a, T b) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		return static_cast<T>(
		    static_cast<U>(a) & (~static_cast<U>(b)));
	} else {
		return a & (~b);
	}
}

template <bool boolean, bit_eligible T>
void set(T& a, T b) {
	if constexpr (boolean) {
		combine(a, b); // set true
	} else {
		erase(a, b); // set false
	}
}
template <bool boolean, bit_eligible T>
T setted(T& a, T b) {
	if constexpr (boolean) {
		return combined(a, b); // set true
	} else {
		return erased(a, b); // set false
	}
}

template <bit_eligible T>
void set(T& a, T b, bool boolean) {
	if (boolean) {
		combine(a, b); // set true
	} else {
		erase(a, b); // set false
	}
}
template <bit_eligible T>
T setted(T& a, T b, bool boolean) {
	if (boolean) {
		return combined(a, b); // set true
	} else {
		return erased(a, b); // set false
	}
}

template <bit_eligible T>
bool includes(T a, T b) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		return static_cast<U>(a) & static_cast<U>(b) == static_cast<U>(b);
	} else {
		return (a & b) == b;
	}
}
template <bit_eligible T>
bool contains(T a, T b) { return includes(a, b); }

template <bit_eligible T>
bool excludes(T a, T b) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		return !(static_cast<U>(a) & static_cast<U>(b));
	} else {
		return !(a & b);
	}
}
template <bit_eligible T>
bool contains_none(T a, T b) { return excludes(a, b); }
} // namespace tx::bit