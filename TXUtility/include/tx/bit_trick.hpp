// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXUtility

#pragma once
#include "impl/basic_utils.hpp"
#include "tx/type_traits.hpp"

#include <concepts>

namespace tx::bit {

template <class T>
concept bit_eligible = std::integral<T> || std::is_enum_v<T>;

template <class T>
struct underlying {
	static_assert(tx::false_v<T>,
	              "tx::bit::underlying: unsupported type.");
};
template <class T>
    requires std::is_enum_v<T>
struct underlying<T> {
	using type = std::underlying_type_t<T>;
};
template <class T>
    requires(!std::is_enum_v<T>) && std::is_integral_v<T>
struct underlying<T> {
	using type = T;
};

template <class T>
using underlying_t = typename underlying<T>::type;

// **************** setters ****************

template <bit_eligible T>
T combined(T target, T val) {
	using U = underlying_t<T>;
	const U vtarget = static_cast<U>(target);
	const U vval = static_cast<U>(val);
	return static_cast<T>(
	    vtarget | vval);
}
template <bit_eligible T>
void combine(T& target, T val) { target = combined(target, val); }

template <bit_eligible T>
T erased(T target, T val) {
	using U = underlying_t<T>;
	const U vtarget = static_cast<U>(target);
	const U vval = static_cast<U>(val);
	return static_cast<T>(
	    vtarget & (~vval));
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
void set(T& target, T val) { target = setted<boolean>(target, val); }

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
T setted(T target, T val, T mask) {
	using U = underlying_t<T>;
	const U vtarget = static_cast<U>(target);
	const U vval = static_cast<U>(val);
	const U vmask = static_cast<U>(mask);
	return static_cast<T>(
	    (vmask & vval) | (~vmask & vtarget));
}
// set bit mask
template <bit_eligible T>
void set(T& target, T val, T mask) { target = setted(target, val, mask); }

// flip each bit
template <bit_eligible T>
T flipped(T target) {
	using U = underlying_t<T>;
	const U vtarget = static_cast<U>(target);
	return static_cast<T>(
	    ~vtarget);
}
// flip each bit
template <bit_eligible T>
void flip(T& target) { target = flipped(target); }

/**
 * @param target the data variable
 * @param mask the bits that will be flipped
 * @note for every `1` bit in mask, the same bit at `target` will be flipped
 */
template <bit_eligible T>
T flipped(T target, T mask) {
	using U = underlying_t<T>;
	const U vtarget = static_cast<U>(target);
	const U vmask = static_cast<U>(mask);
	return static_cast<T>(
	    vtarget ^ vmask);
}
/**
 * @param target the data variable
 * @param mask the bits that will be flipped
 * @note for every `1` bit in mask, the same bit at `target` will be flipped
 */
template <bit_eligible T>
void flip(T& target, T mask) { target = flipped(target, mask); }

// **************** getters ****************

template <bit_eligible T>
bool includes(T target, T val) {
	using U = underlying_t<T>;
	const U vtarget = static_cast<U>(target);
	const U vval = static_cast<U>(val);
	return (vtarget & vval) == vval;
}
template <bit_eligible T>
bool contains_all(T target, T val) { return includes(target, val); }

template <bit_eligible T>
bool overlaps(T target, T val) {
	using U = underlying_t<T>;
	const U vtarget = static_cast<U>(target);
	const U vval = static_cast<U>(val);
	return (vtarget & vval) != U{ 0 };
}
template <bit_eligible T>
bool contains_any(T target, T val) {
	return overlaps(target, val);
}

template <bit_eligible T>
bool excludes(T target, T val) {
	return !overlaps(target, val);
}
template <bit_eligible T>
bool contains_none(T target, T val) { return excludes(target, val); }
} // namespace tx::bit

/**
 * Note for whoever wants to commit:
 * Please change API nameing:
 * - fix the `setted` English error. Suggestion: use `*_copy` for every of them. remember to keep consistency.
 * - delete the `includes`, `overlaps`, `excludes` API alias, and just use `contains_*`
 * I cannot do this myself because TX_Jerry is stopping me to do so.
 * Thank you very much!
 */