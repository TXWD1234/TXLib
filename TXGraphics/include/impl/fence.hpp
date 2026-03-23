// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: fence.hpp

#pragma once
#include "impl/basic_gl_utils.hpp"

namespace tx::RenderEngine {
class Fence {
public:
	Fence() : handle(gl::fenceSync(0x9117 /* GL_SYNC_GPU_COMMANDS_COMPLETE */, 0)) {}
	~Fence() {
		if (handle) gl::deleteSync(handle);
	}

	// Disable Copy & Move Semantics
	Fence(const Fence&) = delete;
	Fence& operator=(const Fence&) = delete;
	Fence(Fence&& other) noexcept : handle(other.handle) {
		other.handle = nullptr;
	}
	Fence& operator=(Fence&& other) noexcept {
		if (this != &other) {
			if (handle) gl::deleteSync(handle);
			handle = other.handle;
			other.handle = nullptr;
		}
		return *this;
	}

	// query if passed
	bool isFinished() const {
		if (!handle) return true;
		u32 result = gl::clientWaitSync(handle, 0, 0);
		return result == 0x911A /* GL_ALREADY_SIGNALED */ || result == 0x911C /* GL_CONDITION_SATISFIED */;
	}

private:
	gl::sync_t handle = nullptr;
};
} // namespace tx::RenderEngine