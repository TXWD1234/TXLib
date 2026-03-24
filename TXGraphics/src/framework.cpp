// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: framework.cpp

#include "impl/framework/framework.hpp"

namespace tx::RenderEngine {

bool MakeWindow::make(GLFWwindow*& window) {
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHintString(GLFW_WAYLAND_APP_ID, "TXStudio_Project"); // set window class for hyprland

	// full screen
	GLFWmonitor* monitor = NULL;
	const GLFWvidmode* mode = NULL;
	if (fullscreen != FullscreenConfig::None) {
		monitor = glfwGetPrimaryMonitor();
		mode = glfwGetVideoMode(monitor);
		dimension.setX(mode->width);
		dimension.setY(mode->height);
	}
	if (fullscreen == FullscreenConfig::Windowed) {
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	}

	window = glfwCreateWindow(
	    dimension.x(), dimension.y(), title.c_str(),
	    fullscreen == FullscreenConfig::Exclusive ? monitor : NULL,
	    NULL);

	if (!window) {
		return 0;
	}

	glfwMakeContextCurrent(window);

	if (fullscreen == FullscreenConfig::Windowed) {
		glfwSetWindowPos(window, 0, 0);
	} else {
		glfwSetWindowPos(window, pos.x(), pos.y());
	}

	return 1;
}

} // namespace tx::RenderEngine