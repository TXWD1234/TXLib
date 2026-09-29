// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/allocator.hpp"
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include <type_traits>
#include <concepts>
#include <span>

// ===========================================================
// **************** Overlay Pattern Utilities ****************
// ===========================================================
namespace tx::impl {

// ################ Type Traits & Concepts ################

template <class O>
concept overlay = requires(O o) {
	{ o.valid() } -> std::same_as<bool>;
	o.destruct();
	typename O::StateStorage;

	/**
	 * State_impl
	 * Parameters - optional
	 * 
	 */
};

template <class O>
concept overlay_parameterized =
    overlay<O> &&
    requires {
	    typename O::Parameters;
    };

template <impl::overlay_parameterized O>
using overlay_parameter_object_t = typename O::Parameters;

template <class T, class O>
concept overlay_parameter_object =
    impl::overlay_parameterized<O> &&
    std::same_as<T, impl::overlay_parameter_object_t<O>>;

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
// Overlay Inlined
/**
 * The normal way of using an overlay.
 * The intended way to create an overlay object.
 */
template <template <class...> class Overlay, class T, class... TArgs>
// requires
class OverlayInlined
    : public Overlay<
          T, OverlayInlinedImpl<OverlayInlined<Overlay, T, TArgs...>>, TArgs...> {
private:
	using Impl = OverlayInlinedImpl<OverlayInlined>;
	using Base = Overlay<T, Impl, TArgs...>;
	using State = typename Base::State_impl;
	friend Base;

private:
	/**
	 * Since if I try to make this universal, it will just become an ungodly
	 * mess of template spaghetti, I intentially choosed hard coding.
	 * Currently it supports single buffer and double buffer as in Data-Meta
	 * pattern.
	 * Adding a buffer is as simple as writing another boolean trait, add a few
	 * ctors, and update the MMW accordingly.
	 */

	template <class... Args>
	struct SingleBufferTrait {
		static constexpr bool value =
		    std::constructible_from<State, T*, u32, Args...>;
	};
	template <class... Args>
	static constexpr bool SingleBuffer = SingleBufferTrait<Args...>::value;

	template <class MetaT, class... Args>
	struct DataMetaBufferTrait {
		static constexpr bool value =
		    std::constructible_from<State, T*, u32, MetaT*, u32, Args...>;
	};
	template <class MetaT, class... Args>
	static constexpr bool DataMetaBuffer = DataMetaBufferTrait<Args...>::value;

public:
	// single buffer

	template <class... Args>
	OverlayInlined(T* bufferPtr, u32 bufferSize, Args&&... args)
	    requires SingleBuffer<Args...>
	    : Base(Impl(bufferPtr, bufferSize, std::forward<Args>(args)...)) {}

	template <class... Args>
	OverlayInlined(std::span<T> buffer, Args&&... args)
	    requires SingleBuffer<Args...>
	    : Base(Impl(buffer.data(), buffer.size(), std::forward<Args>(args)...)) {}

	// double buffer (data, meta)

	template <class MetaT, class... Args>
	OverlayInlined(T* dataBufferPtr, u32 dataBufferSize,
	               MetaT* metaBufferPtr, u32 metaBufferSize, Args&&... args)
	    requires DataMetaBuffer<MetaT, Args...>
	    : Base(Impl(dataBufferPtr, dataBufferSize,
	                metaBufferPtr, metaBufferSize, std::forward<Args>(args)...)) {}

	template <class MetaT, class... Args>
	OverlayInlined(std::span<T> dataBuffer,
	               std::span<MetaT> metaBuffer, Args&&... args)
	    requires DataMetaBuffer<MetaT, Args...>
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
// Overlay Alias
/**
 * Handle object for concurrent access
 * Slightly slower then Inlined due to pointer indirection 
 */
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

// Memory Managing Wrapper
/**
 * The high-level std::vector-like wrapper of overlays
 * This class manages memory and automatically resize when buffer is full
 * 
 * Template Parameters:
 * - `Allocator`: The last parameter of the template. The type does not matter.
 *                Default is std::allocator<T>
 */
template <template <class...> class Overlay, class T, class... TArgs>
class OverlayMMW : OverlayInlined<Overlay, T, TArgs...> {
private:
	// Variadic template helpers for allocator parameter

	template <class... Args>
	static constexpr bool LastArgIsAlloc =
	    tx::type_list_count_v<tx::type_list_t<Args...>> &&
	    tx::allocator<tx::type_list_back_t<tx::type_list_t<Args...>>>;



private:
	using Base = OverlayInlined<Overlay, T, TArgs...>;
	using Allocator = std::conditional_t<
	    LastArgIsAlloc<TArgs...>,
	    tx::type_list_back_t<tx::type_list_t<TArgs...>>, std::allocator<T>>;

public:
	template <class... Args>
	    requires impl::overlay_parameterized<Base>
	OverlayMMW(
	    u32 bufferSize,
	    const impl::overlay_parameter_object_t<Base>& param,
	    Allocator = Allocator{})
	    : Base(nullptr, bufferSize, param) {}

private:
};

} // namespace tx::impl