// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXJson

#pragma once
#include "impl/basic_utils.hpp"
#include <concepts>
#include <string>
#include <type_traits>

namespace tx {

template <class T>
struct json_type {
	using type = T; // Fallback: return the original type (e.g., JsonArray, JsonObject)
};

// Map all integer types (u32, u64, int16_t, etc.) EXCEPT bool to `int`
template <class T>
    requires(std::is_integral_v<T> && !std::is_same_v<T, bool>)
struct json_type<T> {
	using type = int;
};

// Map all floating-point types (double, long double) to `float`
template <class T>
    requires std::is_floating_point_v<T>
struct json_type<T> {
	using type = float;
};

// Map anything constructible to std::string (const char*, string_view) to `std::string`
template <class T>
    requires(std::is_constructible_v<std::string, T> && !std::is_arithmetic_v<T>)
struct json_type<T> {
	using type = std::string;
};

template <class T>
using json_type_t = typename json_type<T>::type;

} // namespace tx