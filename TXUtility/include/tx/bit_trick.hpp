// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXUtility

#pragma once
#include "impl/basic_utils.hpp"
#include "tx/type_traits.hpp"

#include <concepts>

namespace tx::bit {

template <class T>
concept bit_eligible = std::integral<T> || std::is_enum_v<T>;

// **************** setters ****************

template <bit_eligible T>
T combined(T target, T val) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		return static_cast<T>(
		    static_cast<U>(target) | static_cast<U>(val));
	} else
		return target | val;
}
template <bit_eligible T>
void combine(T& target, T val) { target = combined(target, val); }

template <bit_eligible T>
T erased(T target, T val) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		return static_cast<T>(
		    static_cast<U>(target) & (~static_cast<U>(val)));
	} else {
		return target & (~val);
	}
}
template <bit_eligible T>
void erase(T& target, T val) { target = erased(target, val); }

// set_true / set_false
template <bool boolean, bit_eligible T>
T setted(T target, T val) {
	if constexpr (boolean) {
		return combined(target, val); // set true
	} else {
		return erased(target, val); // set false
	}
}
// set_true / set_false
template <bool boolean, bit_eligible T>
void set(T& target, T val) { target = set<boolean>(target, val); }

// set_true / set_false
template <bit_eligible T>
T setted(T target, T val, bool boolean) {
	if (boolean) {
		return combined(target, val); // set true
	} else {
		return erased(target, val); // set false
	}
}
// set_true / set_false
template <bit_eligible T>
void set(T& target, T val, bool boolean) { target = setted(target, val, boolean); }

// set bit mask
template <bit_eligible T>
void setted(T target, T val, T mask) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		U UMask = static_cast<U>(mask);
		return static_cast<T>(
		    (UMask & static_cast<U>(val)) | (~UMask & static_cast<U>(target)));
	} else {
		return (mask & val) | (~mask & target);
	}
}
// set bit mask
template <bit_eligible T>
void set(T& target, T val, T mask) { target = setted(target, val, mask); }

// flip each bit
template <bit_eligible T>
T fliped(T target) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		return static_cast<T>(~static_cast<U>(target));
	} else {
		return ~target;
	}
}
// flip each bit
template <bit_eligible T>
void flip(T& target) { target = fliped(target); }

/**
 * @param target the data variable
 * @param mask the bits that will be fliped
 * @note for every `1` bit in mask, the same bit at `target` will be fliped
 */
template <bit_eligible T>
T fliped(T target, T mask) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		return setted(
		    static_cast<U>(target),
		    ~static_cast<U>(target),
		    static_cast<U>(mask));
	} else {
		return setted(target, ~target, mask);
	}
}
/**
 * @param target the data variable
 * @param mask the bits that will be fliped
 * @note for every `1` bit in mask, the same bit at `target` will be fliped
 */
template <bit_eligible T>
void flip(T& target, T mask) { target = fliped(target, mask); }

// **************** getters ****************

template <bit_eligible T>
bool includes(T target, T val) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		return static_cast<U>(target) & static_cast<U>(val) == static_cast<U>(val);
	} else {
		return (target & val) == val;
	}
}
template <bit_eligible T>
bool contains(T target, T val) { return includes(target, val); }

template <bit_eligible T>
bool excludes(T target, T val) {
	if constexpr (std::is_enum_v<T>) {
		using U = std::underlying_type_t<T>;
		return !(static_cast<U>(target) & static_cast<U>(val));
	} else {
		return !(target & val);
	}
}
template <bit_eligible T>
bool contains_none(T target, T val) { return excludes(target, val); }
} // namespace tx::bit