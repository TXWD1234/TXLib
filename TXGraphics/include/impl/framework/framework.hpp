// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: framework.hpp

#pragma once
#include "tx/math.h"
#include "tx/type_traits.hpp"
#include "impl/gl_core/basic_gl_utils.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
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

	int getWidth() const noexcept { return dimension.x(); }
	int getHeight() const noexcept { return dimension.y(); }
	int getPosX() const noexcept { return pos.x(); }
	int getPosY() const noexcept { return pos.y(); }

	MakeWindow& setPos(int x, int y) noexcept {
		pos.setX(x);
		pos.setY(y);
		return *this;
	}
	MakeWindow& setPos(const Coord& in_pos) noexcept {
		pos = in_pos;
		return *this;
	}

	MakeWindow& setWidth(int w) noexcept {
		dimension.setX(w);
		return *this;
	}
	MakeWindow& setHeight(int h) noexcept {
		dimension.setY(h);
		return *this;
	}
	MakeWindow& setSize(int w, int h) noexcept {
		dimension.setX(w);
		dimension.setY(h);
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

template <u32 mode, std::invocable UpdateCallback, std::invocable RenderCallback>
class Framework {
public:
	Framework() {}
	template <std::invocable U, std::invocable R,
	          std::invocable<GLFWwindow*&> GLFWWindowInitializer = MakeWindow>
	Framework(
	    U&& in_updateCallback,
	    R&& in_renderCallback,
	    GLFWWindowInitializer in_makeWindow = MakeWindow{},
	    double in_FixedTickrate = 60.0,
	    double in_MaxAccumulatorMultiplier = 5.0)
	    : updateCb(std::forward<U>(in_updateCallback)),
	      renderCb(std::forward<R>(in_renderCallback)),
	      m_valid(in_makeWindow(this->window)),
	      FixedTickrate(in_FixedTickrate),
	      TickIntervalTime(1.0 / FixedTickrate),
	      MaxAccumulatorTime(TickIntervalTime * in_MaxAccumulatorMultiplier) {}

	void run() {
		// Main Loop
		std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
		double accumulator = 0.0;

		std::chrono::steady_clock::time_point lastStatTime = std::chrono::steady_clock::now();

		while (!glfwWindowShouldClose(this->window)) {
			if constexpr (mode & Mode::FixTickRate) {
				std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
				double tick_duration = std::chrono::duration<double>(now - last).count();
				accumulator += tick_duration;
				last = now;
				if (accumulator > this->MaxAccumulatorTime)
					accumulator = this->MaxAccumulatorTime;
				while (accumulator >= this->TickIntervalTime) {
					this->updateCb();

					ups++;
					accumulator -= this->TickIntervalTime;
				}
			} else {
				this->updateCb();
				ups++;
			}

			gl::clear(gl::enums::COLOR_BUFFER_BIT);

			this->renderCb();
			fps++;

			glfwSwapBuffers(this->window);
			glfwPollEvents();

			std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
			if (std::chrono::duration<double>(currentTime - lastStatTime).count() >= 1.0) {
				if constexpr (mode & Mode::PrintFrameRate) {
					std::cout << "FPS: " << fps << " | UPS: " << ups << std::endl;
				}
				fps = 0;
				ups = 0;
				lastStatTime = currentTime;
			}
		}
	}
	~Framework() {
	}

	GLFWwindow* getWindow() { return this->window; }
	bool valid() const { return m_valid; }
	u32 getFPS() const { return fps; }
	u32 getUPS() const { return ups; }

private:
	GLFWwindow* window = nullptr;
	UpdateCallback updateCb;
	RenderCallback renderCb;
	double FixedTickrate = 60.0;
	double TickIntervalTime; // seconds
	double MaxAccumulatorTime;
	u32 fps, ups;

	bool m_valid = 1;
};

} // namespace RenderEngine
} // namespace tx