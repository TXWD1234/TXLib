// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: framework.hpp

#pragma once
#include "tx/math.h"
#include "impl/gl_core/basic_gl_utils.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <string>
#include <type_traits>
#include <utility>

namespace tx {
namespace RenderEngine {

enum class Mode {
	Debug = 0,
	Release = 1
};

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

template <Mode mode, class UpdateCallback, class RenderCallback>
class Framework {
public:
	Framework() {}
	template <class U, class R, class GLFWWindowInitializer = MakeWindow>
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
	      MaxAccumulatorTime(TickIntervalTime * in_MaxAccumulatorMultiplier) {
		static_assert(std::is_invocable_v<U> || std::is_invocable_v<U, u64>, "Update callback must have (tx::u64) or () as parameter. The provided callable was invalid.");
		static_assert(std::is_invocable_v<R>, "Render call back must have no parameter. The provided callable was invalid.");
	}

	void run() {
		// Main Loop
		std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
		double accumulator = 0.0;
		while (!glfwWindowShouldClose(this->window)) {
			if constexpr (mode == Mode::Release) {
				std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
				double tick_duration = std::chrono::duration<double>(now - last).count();
				accumulator += tick_duration;
				last = now;
				if (accumulator > this->MaxAccumulatorTime)
					accumulator = this->MaxAccumulatorTime;
				while (accumulator >= this->TickIntervalTime) {
					this->callUpdateCallback(this->tickCounter);

					this->tickCounter++;
					accumulator -= this->TickIntervalTime;
				}
			} else {
				this->callUpdateCallback(this->tickCounter);
				this->tickCounter++;
			}

			//glClearColor(0.0f, 0.9f, 1.0f, 1.0f);
			gl::clear(0x00004000 /* GL_COLOR_BUFFER_BIT */);

			this->renderCb();

			glfwSwapBuffers(this->window);
			glfwPollEvents();
		}
	}
	~Framework() {
	}

	GLFWwindow* getWindow() { return this->window; }
	bool valid() const { return m_valid; }

private:
	GLFWwindow* window = nullptr;
	UpdateCallback updateCb;
	RenderCallback renderCb;
	double FixedTickrate = 60.0;
	double TickIntervalTime; // seconds
	double MaxAccumulatorTime;

	u64 tickCounter = 0;
	bool m_valid = 1;

	inline void callUpdateCallback(int tickCounter) {
		if constexpr (std::is_invocable_v<UpdateCallback, int>) {
			this->updateCb(tickCounter);
		} else if constexpr (std::is_invocable_v<UpdateCallback>) {
			this->updateCb();
		}
	}
};
template <class UF, class RF>
static inline auto CreateRelease(UF&& ucb, RF&& rcb,
                                 double in_FixedTickrate = 60.0,
                                 double in_MaxAccumulatorMultiplier = 5.0) {
	return Framework<Mode::Release, UF, RF>(
	    std::forward<UF>(ucb),
	    std::forward<RF>(rcb),
	    in_FixedTickrate,
	    in_MaxAccumulatorMultiplier);
}
template <class UF, class RF>
static inline auto CreateDebug(UF&& ucb, RF&& rcb,
                               double in_FixedTickrate = 60.0,
                               double in_MaxAccumulatorMultiplier = 5.0) {
	return Framework<Mode::Debug, UF, RF>(
	    std::forward<UF>(ucb),
	    std::forward<RF>(rcb),
	    in_FixedTickrate,
	    in_MaxAccumulatorMultiplier);
}

} // namespace RenderEngine
} // namespace tx