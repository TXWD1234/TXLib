// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/basic_utils.hpp"
#include <concepts>
#include <array>
#include <bitset>

namespace tx {


/**
 * A helper class wrapping a look up table, to perform fast O(1) exist check
 * Only work best for range that's smaller then 1000.
 * If larger than that, you should consider std::unordered_map or tx::KVMap
 * If memory is major concern, use ValueGroupBitSet instead for 8x smaller memory usage
 * 
 * Usage Syntax:
 * ```cpp
 * ValueGroup<char, 'a', 'A'>::contains('b');
 * ```
 */
template <std::integral T, T... vals>
class ValueGroup {
	static_assert(sizeof...(vals) > 0,
	              "tx::ValueGroup: no value provided");

public:
	using value_type = T;
	ValueGroup() = delete;

	static constexpr bool contains(T val) {
		return inRange(val, m_min, m_max) &&
		       m_table[val - m_min];
	}

private:
	inline static constexpr const T m_min = min<T, vals...>();
	inline static constexpr const T m_max = max<T, vals...>();
	inline static constexpr const size_t m_size = m_max - m_min + 1;

	inline static constexpr const std::array<bool, m_size> m_table = []() -> std::array<bool, m_size> {
		std::array<bool, m_size> table = { false };
		((table[vals - m_min] = true), ...);
		return table;
	}();
};

#if __cplusplus >= 202302L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202302L)
/**
 * A helper class wrapping a look up table, to perform fast O(1) exist check
 * Only work best for range that's smaller then 1000.
 * If larger than that, you should consider std::unordered_map or tx::KVMap
 * If performance is major concern, use ValueGroup instead for faster lookup without bit operation
 * 
 * Usage Syntax:
 * ```cpp
 * ValueGroupBitSet<char, 'a', 'A'>::contains('b');
 * ```
 */
template <std::integral T, T... vals>
class ValueGroupBitSet {
	static_assert(sizeof...(vals) > 0,
	              "tx::ValueGroupBitSet: no value provided");

public:
	using value_type = T;
	ValueGroupBitSet() = delete;

	static constexpr bool contains(T val) {
		return inRange(val, m_min, m_max) &&
		       m_table.test(static_cast<size_t>(val - m_min));
	}

private:
	inline static constexpr const T m_min = min<T, vals...>();
	inline static constexpr const T m_max = max<T, vals...>();
	inline static constexpr const size_t m_size = m_max - m_min + 1;

	inline static constexpr const std::bitset<m_size> m_table = []() -> std::bitset<m_size> {
		std::bitset<m_size> table;
		((table.set(static_cast<size_t>(vals - m_min))), ...);
		return table;
	}();
};
#endif


// utilities
using CharWhiteSpaceGroup = ValueGroup<
    char,
    ' ',
    '\t',
    '\n',
    '\r',
    '\f',
    '\v'>;

} // namespace tx