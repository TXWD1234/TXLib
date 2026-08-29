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
template <tx::invocable_r<bool> Cond, class Message>
    requires tx::invocable_r<Message, std::string_view> ||
             std::convertible_to<Message, std::string_view>
inline void assert_impl(
    Cond&& expr, Message&& msg,
    std::source_location loc = std::source_location::current()) {
	if constexpr (config::enabled_debug && config::enabled_exception) {
		if (!expr()) [[unlikely]] {
			std::string_view text;
			std::string storage;
			if constexpr (tx::invocable_r<Message, std::string_view>) {
				storage = msg();
				text = storage;
			} else {
				text = msg;
			}
			throw tx::assertion_failure(std::format(
			    "[{}:{}] {}: {}",
			    loc.file_name(),
			    loc.line(),
			    loc.function_name(),
			    text));
		}
	}
}
} // namespace impl
} // namespace tx