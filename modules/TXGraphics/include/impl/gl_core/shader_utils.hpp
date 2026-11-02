// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXGraphics

#pragma once
#include "tx/basic_types.hpp"
#include "impl/gl_core/shader.hpp"
#include <cstddef>
#include <iostream>
#include <expected>
#include <optional>

namespace tx::RenderEngine {
namespace impl::details {
struct DefaultLogFunc {
	void operator()(std::string_view str) { std::cout << str; }
};
} // namespace impl::details

template <std::invocable<std::string_view> LogFunc = impl::details::DefaultLogFunc>
struct ShaderLogCallback {
	ShaderLogCallback(std::string_view name, LogFunc logFunc)
	    : m_name(name), m_logFund(std::move(logFunc)) {}
	std::string_view m_name;
	[[no_unique_address]] LogFunc m_logFund;
	void operator()(bool success, std::string_view log) {
		LogFunc{}(std::format(
		    "Compiling {}...\n{}{}\n", m_name, log,
		    success ? "Done." : std::format("Failed to compile {}.", m_name)));
	}
};
template <std::invocable<std::string_view> LogFunc = impl::details::DefaultLogFunc>
struct ProgramLogCallback {
	ProgramLogCallback(std::string_view name, LogFunc logFunc)
	    : m_name(name), m_logFund(std::move(logFunc)) {}
	std::string_view m_name;
	[[no_unique_address]] LogFunc m_logFund;
	void operator()(bool success, std::string_view log) {
		LogFunc{}(std::format(
		    "Linking {}...\n{}{}\n", m_name, log,
		    success ? "Done." : std::format("Failed to link {}.", m_name)));
	}
};

template <std::invocable<std::string_view> LogFunc = impl::details::DefaultLogFunc>
inline std::optional<ShaderProgram> buildShader(
    impl::shader_source auto vertSrc,
    impl::shader_source auto fragSrc,
    std::string_view vertName = "vertex shader",
    std::string_view fragName = "fragment shader",
    LogFunc logFunc = LogFunc{}) {
	VertexShader vert(vertSrc, ShaderLogCallback<LogFunc>{ vertName, logFunc });
	if (!vert.valid()) return std::nullopt;
	FragmentShader frag(fragSrc, ShaderLogCallback<LogFunc>{ fragName, logFunc });
	if (!frag.valid()) return std::nullopt;
	ShaderProgram program(
	    vert, frag,
	    ShaderLogCallback<LogFunc>{
	        std::format("{} and {}", vertName, fragName), logFunc });
	if (!program.valid()) return std::nullopt;
	return program;
}

template <std::invocable<std::string_view> LogFunc = impl::details::DefaultLogFunc>
inline std::optional<ShaderProgram> buildShader(
    std::string_view vertSrc,
    std::string_view fragSrc,
    std::string_view vertName = "vertex shader",
    std::string_view fragName = "fragment shader",
    LogFunc&& logFunc = LogFunc{}) {
	return buildShader(StaticShaderSource(vertSrc), StaticShaderSource(fragSrc),
	                   vertName, fragName, std::forward<LogFunc>(logFunc));
}



} // namespace tx::RenderEngine