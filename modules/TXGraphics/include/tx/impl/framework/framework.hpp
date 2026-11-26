// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXGraphics

#pragma once
#include "tx/basic_types.hpp"
#include "tx/math.h"
#include "tx/type_traits.hpp"
#include "tx/impl/gl_core/basic_gl_utils.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <iostream>
#include <utility>
#include <concepts>

namespace tx {
namespace RenderEngine {
// clang-format off

enum Mode : u32 {
	FixTickRate    = 0b0001,
	PrintFrameRate = 0b0010	
};
// clang-format on

class MakeWindow {
public:
	enum class FullscreenConfig {
		Exclusive, // Exclusive Fullscreen. Fastest but have problem with switching screens
		Windowed, // Windowed/Borderless Fullscreen. Slightly slower but behave like normal window
		None // no fullscreen
	};

public:
	MakeWindow(
	    const Coord& in_windowDimension = Coord{ 949, 949 },
	    const Coord& in_windowPos = Coord{ 5, 42 },
	    const std::string& in_windowTitle = "TXStudio Project",
	    FullscreenConfig fullscreenCfg = FullscreenConfig::None)
	    : dimension(in_windowDimension), pos(in_windowPos), title(in_windowTitle), fullscreen(fullscreenCfg) {}

	bool make(GLFWwindow*& window);
	bool operator()(GLFWwindow*& window) {
		return make(window);
	}

	const Coord& getDimension() const noexcept { return dimension; }
	const Coord& getPos() const noexcept { return pos; }
	const std::string& getTitle() const noexcept { return title; }
	MakeWindow& setTitle(const std::string& in_title) {
		title = in_title;
		return *this;
	}

	int getWidth() const noexcept { return dimension.x; }
	int getHeight() const noexcept { return dimension.y; }
	int getPosX() const noexcept { return pos.x; }
	int getPosY() const noexcept { return pos.y; }

	MakeWindow& setPos(int x, int y) noexcept {
		pos.x = x;
		pos.y = y;
		return *this;
	}
	MakeWindow& setPos(const Coord& in_pos) noexcept {
		pos = in_pos;
		return *this;
	}

	MakeWindow& setWidth(int w) noexcept {
		dimension.x = w;
		return *this;
	}
	MakeWindow& setHeight(int h) noexcept {
		dimension.y = h;
		return *this;
	}
	MakeWindow& setSize(int w, int h) noexcept {
		dimension.x = w;
		dimension.y = h;
		return *this;
	}
	MakeWindow& setSize(const Coord& size) noexcept {
		dimension = size;
		return *this;
	}

	MakeWindow& setFullscreen(FullscreenConfig config) noexcept {
		fullscreen = config;
		return *this;
	}

public:
	Coord dimension, pos;
	std::string title;
	FullscreenConfig fullscreen;
};

struct FrameworkConfig {
	f64 Tickrate = 60.0;
	f64 AccumulatorCeiling = 5.0;
};

template <u32 mode, std::invocable UpdateCallback, std::invocable RenderCallback>
class Framework {
public:
	template <std::invocable<GLFWwindow*&> GLFWWindowInitializer = MakeWindow>
	Framework(
	    UpdateCallback&& updateCallback,
	    RenderCallback&& renderCallback,
	    GLFWWindowInitializer makeWindow = MakeWindow{},
	    FrameworkConfig config = FrameworkConfig{})
	    : updateCb(std::move(updateCallback)),
	      renderCb(std::move(renderCallback)),
	      m_config(config) {
		if (!makeWindow(m_window))
			throw std::runtime_error("Failed to make window.");
	}
	~Framework() = default;

	// default 60.0
	void setTickrate(f64 tickRate) {
		m_config.Tickrate = tickRate;
	}
	f64 getTickrate() { return m_config.Tickrate; }

	void run() {
		// Main Loop
		std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
		f64 accumulator = 0.0;

		std::chrono::steady_clock::time_point lastStatTime = std::chrono::steady_clock::now();

		while (!glfwWindowShouldClose(m_window)) {
			if constexpr (mode & Mode::FixTickRate) {
				std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
				f64 tick_duration = std::chrono::duration<f64>(now - last).count();
				accumulator += tick_duration;
				last = now;
				if (accumulator > tickFindAccCeil_impl())
					accumulator = tickFindAccCeil_impl();
				while (accumulator >= tickFindInterval_impl()) {
					updateCb();

					ups++;
					accumulator -= tickFindInterval_impl();
				}
			} else {
				updateCb();
				ups++;
			}

			gl::clear(gl::enums::COLOR_BUFFER_BIT);

			renderCb();
			fps++;

			glfwSwapBuffers(m_window);
			glfwPollEvents();

			std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
			if (std::chrono::duration<f64>(currentTime - lastStatTime).count() >= 1.0) {
				if constexpr (mode & Mode::PrintFrameRate) {
					std::cout << "FPS: " << fps << " | UPS: " << ups << std::endl;
				}
				fps = 0;
				ups = 0;
				lastStatTime = currentTime;
			}
		}
	}

	GLFWwindow* getWindow() { return m_window; }
	u32 getFPS() const { return fps; }
	u32 getUPS() const { return ups; }

private:
	GLFWwindow* m_window = nullptr;
	UpdateCallback updateCb;
	RenderCallback renderCb;

	FrameworkConfig m_config;

	u32 fps = 0, ups = 0;

private:
	// ================ Helper ================

	f64 tickFindInterval_impl() {
		return 1.0 / m_config.Tickrate;
	}
	f64 tickFindAccCeil_impl() {
		return tickFindInterval_impl() * m_config.AccumulatorCeiling;
	}
};

} // namespace RenderEngine
} // namespace tx