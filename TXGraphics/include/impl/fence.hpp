// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: fence.hpp

#pragma once
#include "impl/basic_gl_utils.hpp"

namespace tx::RenderEngine {
class Fence {
public:
	Fence() : handle(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0)) {}
	~Fence() {
		if (handle) glDeleteSync(handle);
	}

	// Disable Copy & Move Semantics
	Fence(const Fence&) = delete;
	Fence& operator=(const Fence&) = delete;
	Fence(Fence&& other) noexcept : handle(other.handle) {
		other.handle = nullptr;
	}
	Fence& operator=(Fence&& other) noexcept {
		if (this != &other) {
			if (handle) glDeleteSync(handle);
			handle = other.handle;
			other.handle = nullptr;
		}
		return *this;
	}

	// query if passed
	bool isFinished() const {
		if (!handle) return true;
		u32 result = glClientWaitSync(handle, 0, 0);
		return result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED;
	}

private:
	GLsync handle = nullptr;
};
} // namespace tx::RenderEngine