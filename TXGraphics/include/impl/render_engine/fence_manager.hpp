// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: fence_manager.hpp

#pragma once
#include "impl/render_engine/basic_gl_utils.hpp"
#include "impl/render_engine/fence.hpp"
#include <tuple>
#include <deque>

namespace tx::RenderEngine {

template <class... FuncTypes>
class FenceManager {
public:
	FenceManager() {
		operationQueue.emplace_back();
	}

	~FenceManager() {
		Fence stallFence;
		while (!stallFence.isFinished()) {} // Block until GPU is completely idle
		for (auto& operationBuffer : operationQueue) {
			executeOperationBuffer_impl(operationBuffer);
		}
	}
	// call this before draw call
	void update() {
		fenceQueue.emplace_back();
		operationQueue.emplace_back();
		resolveFences_impl();
	}

	template <class T>
	void addOperation(T&& operation) {
		std::get<std::vector<std::decay_t<T>>>(operationQueue.back()).push_back(std::forward<T>(operation));
	}

private:
	using OperationBuffer_t_impl = std::tuple<
	    std::vector<FuncTypes>...>;

	std::deque<Fence> fenceQueue;
	std::deque<OperationBuffer_t_impl> operationQueue;

	void resolveFences_impl() {
		while (!fenceQueue.empty() && fenceQueue.front().isFinished()) {
			executeOperationBuffer_impl(operationQueue.front());
			fenceQueue.pop_front();
			operationQueue.pop_front();
		}
	}
	void executeOperationBuffer_impl(OperationBuffer_t_impl& operationBuffer) {
		std::apply(
		    [&](auto&&... buffers) {
			    ([&](auto& buffer) {
				    for (auto& i : buffer) {
					    i();
				    }
			    }(buffers),
			     ...);
		    },
		    operationBuffer);
	}
};

} // namespace tx::RenderEngine