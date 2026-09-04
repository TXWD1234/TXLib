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
struct CompileTimeString {
	std::string_view m_str;
	template <std::convertible_to<std::string_view> T>
	consteval CompileTimeString(T str) : m_str(str) {}
};
/**
 * --- Assertion Documentation ---
 * 
 * Assertions only trigger in debug build, in release build they are completely
 * gone and thereby producing zero runtime overhead.
 * The expression to be asserted (@param expr) is always passed as a lambda, so
 * that it's only evaluated in debug build, ensuring zero overhead in release.
 * The message (@param msg) however, can either be passed as a lambda which
 * returns something convertible to std::string_view (in the case of formatting
 * required), or a compile time string literal for a plain message.
 * The final error message is automaticly formatted, and contain information
 * about the assert site:
 * [<file-name>:<line>] <function-name>: <message>
 * 
 * --- Message Convention Specification ---
 * 
 * Error message is formatted like so:
 * <error-type>. [root-cause]. [variable-value]; [variable-value]...
 * Error type don't have to be rigid types, but error with similar behavior
 * (such as "Index out of range") should have consistent <error-type> string.
 */
template <tx::invocable_r<bool> Cond,
          tx::invocable_r<std::string_view> Message>
constexpr void assert_impl(
    Cond&& expr,
    Message&& msg,
    std::source_location loc = std::source_location::current()) {
	if constexpr (config::enabled_debug && config::enabled_exception) {
		if (!expr()) [[unlikely]] {
			throw tx::assertion_failure(std::format(
			    "[{}:{}] {}: {}",
			    loc.file_name(),
			    loc.line(),
			    loc.function_name(),
			    msg()));
		}
	}
}
template <tx::invocable_r<bool> Cond>
constexpr void assert_impl(
    Cond&& expr,
    CompileTimeString msg,
    std::source_location loc = std::source_location::current()) {
	if constexpr (config::enabled_debug && config::enabled_exception) {
		if (!expr()) [[unlikely]] {
			throw tx::assertion_failure(std::format(
			    "[{}:{}] {}: {}",
			    loc.file_name(),
			    loc.line(),
			    loc.function_name(),
			    msg.m_str));
		}
	}
}

template <class Expr, class Msg>
struct AssertPreset {
	[[no_unique_address]] Expr expr;
	[[no_unique_address]] Msg msg;
};
// wrapper function for the preset pattern
template <class Expr, class Msg>
constexpr void assert_impl(AssertPreset<Expr, Msg> preset) {
	impl::assert_impl(
	    preset.expr,
	    preset.msg);
}

namespace assert {
inline auto out_of_range(u32 size, u32 index) {
	return AssertPreset{
		[=] { return index < size; },
		[=] { return std::format("Index out of range. index = {}; size = {}", index, size); }
	};
}
inline auto buffer_empty(u32 size) {
	return AssertPreset{
		[=] { return size; },
		[=] { return "Buffer is empty."; }
	};
}
inline auto buffer_full(u32 size, u32 capacity) {
	return AssertPreset{
		[=] { return size < capacity; }, // not <= because this assert is intended
		// to be called before the size increasing operation (such as insertion)
		[=] { return "Buffer is full."; }
	};
}
// Valid Check targeted specificly for objects of the Overlay Pattern
// The object provided must provide public method of `valid`
template <class T>
    requires requires(T obj) {
	    { obj.valid() } -> std::same_as<bool>;
    }
inline auto object_valid(T* obj) {
	return AssertPreset{
		[=] { return obj->valid(); },
		[=] { return "Invalid object."; }
	};
}
} // namespace assert

} // namespace impl
} // namespace tx