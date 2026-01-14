// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// File: TXLib_Graphics

#pragma once
#include "libs/TXLib/txlib.hpp"
#include "libs/GLFW/include/glfw3.h"

// txglib ************************************************************************************************************** TXGLib
namespace tx {
	// Fake OpenGL APIs
	inline void glColorRGB(const RGB& in) {
		glColor3f(in.r(), in.g(), in.b());
	}
	inline void glVertexVec(const vec2 in) {
		glVertex2f(in.x(), in.y());
	}
	// Fake GLFW APIs
	template<class T, void(T::*Callback)(GLFWwindow*, int, int, int, int)>
	void glfwSetKeyCallback(GLFWwindow* window, T* _this) {
		glfwSetWindowUserPointer(window, _this);

		glfwSetKeyCallback(window,
			[](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				T* obj = static_cast<T*>(glfwGetWindowUserPointer(window));
				if (obj) {
					(obj->*Callback)(window, key, scancode, action, mods);
				}
			}
		);
	}
	vec2 glfwGetCursorPos(GLFWwindow* window) {
		static int width;
		static int height;
		static bool inited = [&]() {
			glfwGetWindowSize(window, &width, &height);
			width *= 0.5;
			height *= 0.5;
			return 1;
			}();
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		x = (x - width) / (width);
		y = -(y - height) / (height);
		return vec2(x, y);
	}

	inline const RGB& getBWColor(bool in) { return in ? tx::White : tx::Black; } // 1 = white, 0 = black
	void initCircleVertices(vector<vec2>& pos, int sideAmount) {
		float degree = 0.0;
		float degreeIncrement = 360.0 / sideAmount * ONE_DEGREE;
		pos.reserve(sideAmount);
		for(int i = 0; i < sideAmount; i++) {
			pos.push_back(tx::orbitalPos(degree));
			degree += degreeIncrement;
		}
	}
	void drawPolygon(const vec2& pos, const vector<vec2>& vertices, float radius) {
		vec2 now(pos.x() + vertices[0].x() * radius, pos.y() + vertices[0].y() * radius);
		for (int i = 0; i < vertices.size(); i++) {
			int next = i + 1 == vertices.size() ? 0 : i + 1;
			glVertex2f(pos.x(), pos.y());
			glVertex2f(now.x(), now.y());
			now.setX(pos.x() + vertices[next].x() * radius); now.setY(pos.y() + vertices[next].y() * radius);
			glVertex2f(now.x(), now.y());
		}
	}
	void drawPolygon(const vec2& pos, float radius, int sideAmount) {
		static vector<vec2> vertices; // { { x, y } }
		static bool inited = (initCircleVertices(vertices, sideAmount), 1);
		if (sideAmount != vertices.size()) {
			vertices.clear();
			initCircleVertices(vertices, sideAmount);
		}
		drawPolygon(pos, vertices, radius);
	}
	void drawCircle(const vec2& pos, float radius) {
		static vector<vec2> vertices; // { { x, y } }
		static bool inited = (initCircleVertices(vertices, 60), 1);
		drawPolygon(pos, vertices, radius);
	}
	void drawQuad(const vec2& a, const vec2& b, const vec2& c, const vec2& d) {
		glVertexVec(a);
		glVertexVec(b);
		glVertexVec(c);
		glVertexVec(a);
		glVertexVec(c);
		glVertexVec(d);
	}
	void drawRect(const vec2& topLeft, float width, float height) {
		drawQuad(
			topLeft,
			topLeft.offsetX(width),
			topLeft.offsetX(width).offsetY(-height),
			topLeft.offsetY(-height)
		);
	}
	void drawDomain(const vec2& center, float width, float height, float lineWidth = 0.001f) {
		float halfWidth = width * 0.5, halfHeight = height * 0.5;
		vec2 innerTopLeft = center.offset(-halfWidth, halfHeight);
		vec2 outerTopLeft = innerTopLeft.offset(-lineWidth, lineWidth);
		drawRect(outerTopLeft, lineWidth, height + 2 * lineWidth);
		drawRect(outerTopLeft.offsetX(width + lineWidth), lineWidth, height + 2 * lineWidth);
		drawRect(innerTopLeft.offsetY(lineWidth), width, lineWidth);
		drawRect(innerTopLeft.offsetY(-height), width, lineWidth);
	}
	inline void drawLine(const vec2& pa, const vec2& pb, float halfLineWidth = 0.001f) {
		vec2 normalVecLeft = mirror(unify(pa - pb)) * halfLineWidth; normalVecLeft.setX(normalVecLeft.getX() * -1.0f);
		drawQuad(
			pa + normalVecLeft, // a
			pb + normalVecLeft, // c
			pb - normalVecLeft, // d
			pa - normalVecLeft  // b
		);
	}
	inline void drawJointedLine(const vec2& pa, const vec2& pb, float lineWidth = 0.001f) {
		float halfWidth = lineWidth * 0.5f;
		drawCircle(pa, halfWidth);
		drawCircle(pb, halfWidth);
		drawLine(pa, pb, halfWidth);
	}
	inline void drawCircularDomain(const vec2& pos, float radius, float lineWidth = 0.003f) {
		static vector<vec2> vertices; // { { x, y } }
		static bool inited = (initCircleVertices(vertices, 60), 1);
		float halfWidth = lineWidth * 0.5f;
		vec2 now = pos + vertices[0] * radius;
		for (int i = 0; i < vertices.size(); i++) {
			int nextIndex = i + 1 == vertices.size() ? 0 : i + 1;
			vec2 next = pos + vertices[nextIndex] * radius;
			drawLine(now, next, halfWidth);
			//drawCircle(now, halfWidth);
			now = next;
		}
	}















	namespace RenderEngine {
		enum class Mode {
			Debug = 0,
			Release = 1
		};

		template<Mode mode, class UpdateCallback, class RenderCallback>
		class Framework {
		public:

			template<class U, class R>
			Framework(
				U&& in_updateCallback,
				R&& in_renderCallback,
				double in_FixedTickrate = 60.0,
				double in_MaxAccumulatorMultiplier = 5.0) :
				updateCb(std::forward<U>(in_updateCallback)),
				renderCb(std::forward<R>(in_renderCallback)),
				FixedTickrate(in_FixedTickrate),
				TickIntervalTime(1.0 / FixedTickrate),
				MaxAccumulatorTime(TickIntervalTime* in_MaxAccumulatorMultiplier)
			{
				static_assert(std::is_invocable_v<U> || std::is_invocable_v<U, int>, "Update callback must have (int) or () as parameter. The provided callable was invalid.");
				static_assert(std::is_invocable_v<R>, "Render call back must have no parameter. The provided callable was invalid.");
				initGLFW();
			}

			void run() {
				// Main Loop
				timeBeginPeriod(1);
				std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
				double accumulator = 0.0;
				while (!glfwWindowShouldClose(this->window)) {
					if constexpr (mode == Mode::Release) {
						std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
						double tick_duration = std::chrono::duration<double>(now - last).count();
						accumulator += tick_duration;
						last = now;
						if (accumulator > this->MaxAccumulatorTime) accumulator = this->MaxAccumulatorTime;
						while (accumulator >= this->TickIntervalTime) {
							this->callUpdateCallback(this->tickCounter);

							this->tickCounter++;
							accumulator -= this->TickIntervalTime;
						}
					}
					else {
						this->callUpdateCallback(this->tickCounter);
						this->tickCounter++;
					}




					//glClearColor(0.80f, 9.0f, 1.0f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT);
					// render start
					glBegin(GL_TRIANGLES);

					this->renderCb();

					// render end
					glEnd();


					glfwSwapBuffers(this->window);
					glfwPollEvents();
				}
			}
			~Framework() {
				if (valid) {
					timeEndPeriod(1);
					glfwTerminate();
				}

			}

			inline GLFWwindow* getWindow() { return this->window; }

		private:
			GLFWwindow* window;
			UpdateCallback updateCb;
			RenderCallback renderCb;
			double FixedTickrate = 60.0;
			double TickIntervalTime; // seconds
			double MaxAccumulatorTime;

			int tickCounter = 0;
			bool valid = 1;

			void initGLFW() {
				if (!glfwInit()) {
					this->valid = 0;
					return;
				}

				glfwWindowHint(GLFW_SAMPLES, 4);
				//GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor(); // main monitor
				//const GLFWvidmode* primaryMonitorMode = glfwGetVideoMode(primaryMonitor);
				//window = glfwCreateWindow(primaryMonitorMode->width, primaryMonitorMode->height, "Project by TX_Jerry", primaryMonitor, NULL);

				this->window = glfwCreateWindow(900, 900, "Project by TX_Jerry", NULL, NULL);
				if (!this->window) {
					glfwTerminate();
					this->valid = 0;
					return;
				}
				glfwMakeContextCurrent(this->window);

				glfwSetWindowPos(this->window, 5, 42);
				//glfwSetWindowPos(window, 0, 0);
			}

			inline void callUpdateCallback(int tickCounter) {
				if constexpr (std::is_invocable_v<UpdateCallback, int>) {
					this->updateCb(tickCounter);
				}
				else if constexpr (std::is_invocable_v<UpdateCallback>) {
					this->updateCb();
				}
			}

		};
		template<class UF, class RF>
		static inline auto CreateRelease(UF&& ucb, RF&& rcb,
			double in_FixedTickrate = 60.0,
			double in_MaxAccumulatorMultiplier = 5.0) {
			return Framework<Mode::Release, UF, RF>(
				std::forward<UF>(ucb),
				std::forward<RF>(rcb),
				in_FixedTickrate,
				in_MaxAccumulatorMultiplier
			);
		}
		template<class UF, class RF>
		static inline auto CreateDebug(UF&& ucb, RF&& rcb,
			double in_FixedTickrate = 60.0,
			double in_MaxAccumulatorMultiplier = 5.0) {
			return Framework<Mode::Debug, UF, RF>(
				std::forward<UF>(ucb),
				std::forward<RF>(rcb),
				in_FixedTickrate,
				in_MaxAccumulatorMultiplier
			);
		}

		





	}
	namespace RE = RenderEngine;






}