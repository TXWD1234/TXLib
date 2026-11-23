// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXGraphics

#pragma once
#include <iostream>
#include <string>
#include <type_traits>

namespace tx::RenderEngine::Log {

template <class... Args>
struct first_is_log {
	inline static constexpr bool value = false;
};

template <class Func, class... Args>
struct first_is_log<Func, Args...> {
	inline static constexpr bool value = std::is_invocable_v<Func, bool, const std::string&>;
};

struct Compilation {
	void operator()(bool success, const std::string& log) {
		std::cerr << "Compiling Shaders...\n";
		if (!success)
			std::cerr << "Error compiling shader.\n";
		std::cerr << log;
		if (success) std::cerr << "Done.\n";
	}
};

struct Linking {
	void operator()(bool success, const std::string& log) {
		std::cerr << "Linking Shaders...\n";
		if (!success)
			std::cerr << "Error linking shaders.\n";
		std::cerr << log;
		if (success) std::cerr << "Done.\n";
	}
};


} // namespace tx::RenderEngine::Log