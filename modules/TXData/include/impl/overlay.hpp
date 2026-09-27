// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include <type_traits>
#include <concepts>
#include <span>

// ================ Overlay Pattern Utilities ================
namespace tx::impl {

// concept for overlay class
template <class O>
concept overlay = requires(O o) {
	{ o.valid() } -> std::same_as<bool>;
	o.destruct();
	typename O::StateStorage;
};


// OverlayInlined's implementation
// do not touch
template <class Derived>
class OverlayInlinedImpl {
	friend Derived;

private:
	using State = typename Derived::State;

private:
	State m_state;

private:
	template <class... Args>
	OverlayInlinedImpl(Args&&... args) : m_state(std::forward<Args>(args)...) {}

private:
	template <class Self>
	tx::const_propagate<Self, State>& operator()(this Self&& self) {
		return self.m_state;
	}
};
template <template <class...> class Overlay, class T, class... TArgs>
// requires
class OverlayInlined
    : public Overlay<
          T, OverlayInlinedImpl<OverlayInlined<Overlay, T, TArgs...>>, TArgs...> {
	/**
	 * This class is always constructed with an existing state of an existing
	 * overlay object, and is not expected to manage the lifetime of the
	 * overlay object. The lifetime of this class is always between the
	 * lifetime of the parent overlay object.
	 */
private:
	using Impl = OverlayInlinedImpl<OverlayInlined>;
	using Base = Overlay<T, Impl, TArgs...>;
	using State = typename Base::State_impl;
	friend Base;

public:
	// single buffer

	template <class... Args>
	OverlayInlined(T* bufferPtr, u32 bufferSize, Args&&... args)
	    requires std::constructible_from<State, T*, u32, Args...>
	    : Base(Impl(bufferPtr, bufferSize, std::forward<Args>(args)...)) {}

	template <class... Args>
	OverlayInlined(std::span<T> buffer, Args&&... args)
	    requires std::constructible_from<State, T*, u32, Args...>
	    : Base(Impl(buffer.data(), buffer.size(), std::forward<Args>(args)...)) {}

	// double buffer (data, meta)

	template <class MetaT, class... Args>
	OverlayInlined(T* dataBufferPtr, u32 dataBufferSize,
	               MetaT* metaBufferPtr, u32 metaBufferSize, Args&&... args)
	    requires std::constructible_from<State, T*, u32, MetaT, u32, Args...>
	    : Base(Impl(dataBufferPtr, dataBufferSize,
	                metaBufferPtr, metaBufferSize, std::forward<Args>(args)...)) {}

	template <class MetaT, class... Args>
	OverlayInlined(std::span<T> dataBuffer,
	               std::span<MetaT> metaBuffer, Args&&... args)
	    requires std::constructible_from<State, T*, u32, MetaT, u32, Args...>
	    : Base(Impl(dataBuffer.data(), dataBuffer.size(),
	                metaBuffer.data(), metaBuffer.size(),
	                std::forward<Args>(args)...)) {}

	// m_state will be default initialized into null state
	OverlayInlined() {}
};

// OverlayAlias's implementation
// do not touch
template <class Derived>
class OverlayAliasImpl {
	friend Derived;

private:
	using State = typename Derived::State;

private:
	State* m_state;

private:
	OverlayAliasImpl(State* statePtr) : m_state(statePtr) {}

private:
	template <class Self>
	tx::const_propagate<Self, State>& operator()(this Self&& self) {
		return *static_cast<tx::const_propagate<Self, State>*>(self.m_state);
	}
};
template <template <class...> class Overlay, class T, class... TArgs>
// requires
class OverlayAlias
    : public Overlay<
          T, OverlayAliasImpl<OverlayAlias<Overlay, T, TArgs...>>, TArgs...> {
	/**
	 * This class is always constructed with an existing state of an existing
	 * overlay object, and is not expected to manage the lifetime of the
	 * overlay object. The lifetime of this class is always between the
	 * lifetime of the parent overlay object.
	 */
private:
	using Impl = OverlayAliasImpl<OverlayAlias>;
	using Base = Overlay<T, Impl, TArgs...>;
	using State = typename Base::State_impl;
	friend Base;

public:
	OverlayAlias(OverlayInlined<Overlay, T, TArgs...>& parent)
	    : Base(Impl(&parent.overlayGetState_impl())) {}

private:
	OverlayAlias(State* state)
	    : Base(Impl(state)) {}
};
} // namespace tx::impl