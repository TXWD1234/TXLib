// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/allocator.hpp"
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include <memory>
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
	 * Internal Utility APIs
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


// ################ Base Subclass / Wrapper Utilities ################

/**
 * Basic subclass of OverlayBase
 * Providing addition to the protected internal utilities to serve all overlay
 * pattern wrappers
 */
template <impl::overlay O, class T>
class OverlayInternal : public O {
private:
	using State = typename O::State_impl;

protected:
	// ================ Buffer Traits ================

	/**
	 * Since if I try to make this universal, it will just become an ungodly
	 * mess of template spaghetti, I intentially choosed hard coding.
	 * Currently it supports single buffer and double buffer as in Data-Meta
	 * pattern.
	 * Adding a buffer is as simple as writing another boolean trait, add a few
	 * ctors, and update the MMW accordingly.
	 */

	struct SingleBufferTrait {
		static constexpr bool value =
		    std::constructible_from<State, T*, u32>;
	};
	template <class MetaT>
	struct DataMetaBufferTrait {
		static constexpr bool value =
		    std::constructible_from<State, T*, u32, MetaT*, u32>;
	};

	static constexpr bool SingleBuffer = SingleBufferTrait::value;
	template <class MetaT>
	static constexpr bool DataMetaBuffer = DataMetaBufferTrait<MetaT>::value;
};

/**
 * Basic subclass of the OverlayBase
 * Providing addition to the public interface of the Base class to serve
 * all overlay pattern wrappers
 */
template <impl::overlay O, class T>
class OverlayInterface : public O {
public:
	// ================ Type Alias ================

	using value_type = T;
};

/**
 * Basic subclass of the OverlayBase
 * Providing addition to the public interface of the Base class for relocation
 * to serve OverlayInlined and OverlayAlias
 */
template <impl::overlay O, class T>
class OverlayRelocationInterface : public O {
public:
	// ================ Relocation ================
};

// ################ Overlay Pattern Wrappers ################

// OverlayInlined's implementation
// do not touch
template <class OInlined>
class OverlayInlinedImpl {
	friend OInlined;

private:
	using State = typename OInlined::State;

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
    : public OverlayRelocationInterface<
          OverlayInterface<
              OverlayInternal<
                  Overlay<T, OverlayInlinedImpl<OverlayInlined<Overlay, T, TArgs...>>, TArgs...>,
                  T>,
              T>,
          T> {
private:
	using Impl = OverlayInlinedImpl<OverlayInlined>;
	using Base =
	    OverlayRelocationInterface<
	        OverlayInterface<
	            OverlayInternal<
	                Overlay<T, Impl, TArgs...>,
	                T>,
	            T>,
	        T>;
	using State = typename Base::State_impl;
	using ParamObj = impl::overlay_parameter_object_t<Base>;
	friend Base;

public:
	// ================ Public Construction Interface ================

	// single buffer
	OverlayInlined(T* bufferPtr, u32 bufferSize)
	    requires Base::SingleBuffer
	    : Base(Impl(bufferPtr, bufferSize)) {}
	OverlayInlined(std::span<T> buffer)
	    requires Base::SingleBuffer
	    : Base(Impl(buffer.data(), buffer.size())) {}

	OverlayInlined(T* bufferPtr, u32 bufferSize, const ParamObj& param)
	    requires Base::SingleBuffer && impl::overlay_parameterized<Base>
	    : Base(Impl(bufferPtr, bufferSize, param)) {}
	OverlayInlined(std::span<T> buffer, const ParamObj& param)
	    requires Base::SingleBuffer && impl::overlay_parameterized<Base>
	    : Base(Impl(buffer.data(), buffer.size(), param)) {}

	// double buffer (data, meta)

	template <class MetaT>
	OverlayInlined(T* dataBufferPtr, u32 dataBufferSize,
	               MetaT* metaBufferPtr, u32 metaBufferSize)
	    requires Base::template
	DataMetaBuffer<MetaT>
	    : Base(Impl(dataBufferPtr, dataBufferSize,
	                metaBufferPtr, metaBufferSize)) {}
	template <class MetaT>
	OverlayInlined(std::span<T> dataBuffer,
	               std::span<MetaT> metaBuffer)
	    requires Base::template
	DataMetaBuffer<MetaT>
	    : Base(Impl(dataBuffer.data(), dataBuffer.size(),
	                metaBuffer.data(), metaBuffer.size())) {}

	template <class MetaT>
	OverlayInlined(T* dataBufferPtr, u32 dataBufferSize,
	               MetaT* metaBufferPtr, u32 metaBufferSize,
	               const ParamObj& param)
	    requires Base::template
	DataMetaBuffer<MetaT>&& impl::overlay_parameterized<Base>
	    : Base(Impl(dataBufferPtr, dataBufferSize,
	                metaBufferPtr, metaBufferSize, param)) {}
	template <class MetaT>
	OverlayInlined(std::span<T> dataBuffer,
	               std::span<MetaT> metaBuffer,
	               const ParamObj& param)
	    requires Base::template
	DataMetaBuffer<MetaT>&& impl::overlay_parameterized<Base>
	    : Base(Impl(dataBuffer.data(), dataBuffer.size(),
	                metaBuffer.data(), metaBuffer.size(), param)) {}

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
    : public OverlayRelocationInterface<
          OverlayInterface<
              OverlayInternal<
                  Overlay<T, OverlayAliasImpl<OverlayAlias<Overlay, T, TArgs...>>, TArgs...>,
                  T>,
              T>,
          T> {
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
class OverlayMMW
    : OverlayInterface<
          OverlayInternal<
              Overlay<T, OverlayInlinedImpl<OverlayInlined<Overlay, T, TArgs...>>, TArgs...>,
              T>,
          T> {
private:
	template <class... Args>
	static constexpr bool LastArgIsAlloc =
	    tx::type_list_count_v<tx::type_list_t<Args...>> &&
	    tx::allocator<tx::type_list_back_t<tx::type_list_t<Args...>>>;

	using Base = OverlayInterface<
	    OverlayInternal<
	        Overlay<T, OverlayInlinedImpl<OverlayInlined<Overlay, T, TArgs...>>, TArgs...>,
	        T>,
	    T>;
	using Allocator = std::conditional_t<
	    LastArgIsAlloc<TArgs...>,
	    tx::type_list_back_t<tx::type_list_t<TArgs...>>, std::allocator<T>>;
	using ParamObj = impl::overlay_parameter_object_t<Base>;

private:
	// ================ Allocation & Reallocation ================

	template <class U>
	using alloc_traits = tx::typed_allocator_traits<Allocator, U>;

	[[no_unique_address]] Allocator m_alloc;

public:
	// ================ Public Construction Interface ================

	OverlayMMW(
	    u32 bufferSize, Allocator alloc = Allocator{})
	    requires Base::SingleBuffer
	    : Base(alloc_traits<T>::allocate(alloc, bufferSize), bufferSize),
	      m_alloc(std::move(alloc)) {}

	OverlayMMW(
	    u32 bufferSize, const ParamObj& param, Allocator alloc = Allocator{})
	    requires Base::SingleBuffer && impl::overlay_parameterized<Base>
	    : Base(alloc_traits<T>::allocate(alloc, bufferSize), bufferSize, param),
	      m_alloc(std::move(alloc)) {}

private:
};

} // namespace tx::impl