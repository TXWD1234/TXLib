// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXGraphics

#pragma once
#include "tx/type_traits.hpp"
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/fence.hpp"
#include <tuple>
#include <deque>

namespace tx::RenderEngine {

template <class... FuncTypes>
class FenceManagerBase {
public:
	FenceManagerBase() {
		operationQueue.emplace_back();
	}

	~FenceManagerBase() {
		Fence stallFence;
		while (!stallFence.isFinished()) {} // Block until GPU is completely idle
		for (auto& operationBuffer : operationQueue) {
			executeOperationBuffer_impl(operationBuffer);
		}
	}

	// Disable Copy
	FenceManagerBase(const FenceManagerBase&) = delete;
	FenceManagerBase& operator=(const FenceManagerBase&) = delete;

	// Move Semantics
	FenceManagerBase(FenceManagerBase&&) noexcept = default;
	FenceManagerBase& operator=(FenceManagerBase&&) noexcept = default;

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

/// @brief A helper callable that forwards a deleter operation to a FenceManager.
/// Used to pass a fence manager to functions that expect a deleter-submitting functor.
template <InstantiationOf<FenceManagerBase> FMT>
struct FMAddOperation {
	FMAddOperation(FMT& in_fm) : fm(in_fm) {}
	FMT& fm;

	template <class Func>
	void operator()(Func&& operation) const {
		fm.addOperation(std::forward<Func>(operation));
	}
};

template <typename FMT>
FMAddOperation(FMT&) -> FMAddOperation<FMT>;

template <InstantiationOf<FenceManagerBase> FMT>
using FMSubmiter = FMAddOperation<FMT>;

using FenceManager = FenceManagerBase<
    RingBufferObjectDeleter,
    RingBufferObjectMarker,
    TextureDeleter>;
using FM = FenceManager;

} // namespace tx::RenderEngine