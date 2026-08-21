// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXFoundation

#pragma once
#include "tx/build_config.hpp"
#include "tx/type_traits.hpp"
#include <stdexcept>

namespace tx::impl {
template <tx::invocable_r<bool> Expr>
inline static void assert_impl(Expr&& expr, const char* message) {
	if constexpr (config::enabled_debug && config::enabled_exception) {
		if (!expr()) [[unlikely]]
			throw std::runtime_error(message);
	}
}
} // namespace tx::impl