// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXFoundation

#pragma once
#include "tx/build_config.hpp"
#include "tx/type_traits.hpp"
#include <concepts>
#include <source_location>
#include <stdexcept>
#include <string_view>
#include <string>
#include <format>

namespace tx {

class exception : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

class assertion_failure : public tx::exception {
public:
	using tx::exception::exception;
};

namespace impl {

struct AssertProxy {
	template <std::convertible_to<std::string_view> T>
	constexpr AssertProxy(
	    T message,
	    std::source_location location =
	        std::source_location::current())
	    : str(message), loc(location) {}

	std::string_view str;
	std::source_location loc;
};

template <tx::invocable_r<bool> Expr, std::convertible_to<std::string_view>... Args>
inline static void assert_impl(Expr&& expr, AssertProxy proxy, const Args&... message) {
	if constexpr (config::enabled_debug && config::enabled_exception) {
		if (!expr()) [[unlikely]] {
			std::string str(proxy.str);
			(str.append(message), ...);
			throw tx::assertion_failure(std::format(
			    "[{}:{}] {}: {}",
			    proxy.loc.file_name(),
			    proxy.loc.line(),
			    proxy.loc.function_name(),
			    str));
		}
	}
}
} // namespace impl
} // namespace tx