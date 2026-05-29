// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/basic_utils.hpp"

namespace tx {
// this is scheduled to move to TXFoundation immediately after TXLib structural refactor
#ifdef NDEBUG
inline constexpr const bool enabled_debug = false;
#else
inline constexpr const bool enabled_debug = true;
#endif

#ifdef __cpp_exceptions
inline constexpr const bool enabled_exception = true;
#else
inline constexpr const bool enabled_exception = false;
#endif

template <tx::invocable_r<bool> Expr>
inline static void assert_impl(Expr&& expr, const char* message) {
	if constexpr (enabled_debug && enabled_exception) {
		if (!expr()) [[unlikely]]
			throw std::runtime_error(message);
	}
}

template <std::input_iterator It>
inline u32 findIteratorIndex(It begin, It it) {
	return static_cast<u32>(std::distance(begin, it)); // O(n)
}
} // namespace tx