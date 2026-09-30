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

// DevNote: eval to void if non-parameterized
template <impl::overlay_parameterized O>
using overlay_parameter_object_t = typename O::Parameters;

template <class T, class O>
concept overlay_parameter_object =
    impl::overlay_parameterized<O> &&
    std::same_as<T, impl::overlay_parameter_object_t<O>>;

// ################ Base Subclass / Wrapper Utilities ################
/**
 * The sequence of applying the wrapper utility classes are non-randomizable.
 * OverlayInternal must be the first subclass around the raw Base class.
 * The rule is: From least specific / most generic (closer to the raw Base
 * class) to most specific / least generic (further from the raw Base class).
 */

/**
 * Basic subclass of OverlayBase
 * Providing addition to the protected internal utilities to serve all overlay
 * pattern wrappers
 */
template <impl::overlay O, class T>
class OverlayInternal : public O {
protected:
	using overlay_internal_tag = void;

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

	static constexpr bool SingleBuffer =
	    std::constructible_from<State, T*, u32>;
	static constexpr bool DataMetaBuffer = requires {
		typename O::meta_type;
		requires std::constructible_from<
		    State, T*, u32, typename O::meta_type*, u32>;
	};

protected:
	// ================ Buffer Trait Utilities ================

	using Meta = typename decltype([] {
		if constexpr (DataMetaBuffer)
			return std::type_identity<typename O::meta_type>{};
		else
			return std::type_identity<void>{};
	}())::type;

protected:
	// ================ Type Alias ================

	using OverlayBase = O;

public:
	using O::O;
};

template <class O>
concept overlay_internal =
    impl::overlay<O> && requires { typename O::overlay_internal_tag; };

/**
 * Basic subclass of the OverlayBase
 * Providing addition to the public interface of the Base class to serve
 * all overlay pattern wrappers
 * 
 * O must be an overlay internal
 */
template <impl::overlay_internal O, class T>
class OverlayInterface : public O {
public:
	// ================ Type Alias ================

	using value_type = T;

public:
	using O::O;
};

/**
 * Basic subclass of the OverlayBase
 * Providing addition to the public interface of the Base class for relocation
 * to serve OverlayInlined and OverlayAlias
 * 
 * O must be an overlay internal
 */
template <impl::overlay_internal O, class T>
class OverlayRelocationInterface : public O {
private:
	using Base = O;
	using Meta = typename Base::Meta;

public:
	// ================ Relocation ================

	/**
	 * Rebind is stubbed for now. The architectural requirement of rebind is
	 * currently way too vague and cannot derive a stable interface. It will be
	 * completed when a demand appears
	 */

	// single buffer
	void relocate(T* bufferPtr, u32 bufferSize)
	    requires Base::SingleBuffer
	{ this->overlaySetBufferState(bufferPtr, bufferSize); }
	void relocate(std::span<T> buffer)
	    requires Base::SingleBuffer
	{ this->overlaySetBufferState(buffer.data(), buffer.size()); }

	// double buffer (data, meta)
	void relocate(T* dataBufferPtr, u32 dataBufferSize,
	              Meta* metaBufferPtr, u32 metaBufferSize)
	    requires Base::DataMetaBuffer
	{ this->overlaySetBufferState(
		dataBufferPtr, dataBufferSize,
		metaBufferPtr, metaBufferSize); }
	void relocate(std::span<T> dataBuffer,
	              std::span<Meta> metaBuffer)
	    requires Base::DataMetaBuffer
	{ this->overlaySetBufferState(
		dataBuffer.data(), dataBuffer.size(),
		metaBuffer.data(), metaBuffer.size()); }

	// ================ Buffer Information ================

	template <std::invocable<T*, u32> Func>
	void getBufferInfo(Func&& f)
	    requires Base::SingleBuffer
	{ this->overlayGetBufferState(std::forward<Func>(f)); }

	template <std::invocable<T*, u32, Meta*, u32> Func>
	void getBufferInfo(Func&& f)
	    requires Base::DataMetaBuffer
	{ this->overlayGetBufferState(std::forward<Func>(f)); }

public:
	using O::O;
};

// ################ Overlay Pattern Wrappers ################

// OverlayInlined's implementation
// do not touch
template <class OInlined>
class OverlayInlinedImpl {
	friend OInlined;
	friend typename OInlined::OverlayBase;

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
	using Meta = typename Base::Meta;
	friend Impl;

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

	OverlayInlined(T* dataBufferPtr, u32 dataBufferSize,
	               Meta* metaBufferPtr, u32 metaBufferSize)
	    requires Base::DataMetaBuffer
	    : Base(Impl(dataBufferPtr, dataBufferSize,
	                metaBufferPtr, metaBufferSize)) {}
	OverlayInlined(std::span<T> dataBuffer,
	               std::span<Meta> metaBuffer)
	    requires Base::DataMetaBuffer
	    : Base(Impl(dataBuffer.data(), dataBuffer.size(),
	                metaBuffer.data(), metaBuffer.size())) {}

	OverlayInlined(T* dataBufferPtr, u32 dataBufferSize,
	               Meta* metaBufferPtr, u32 metaBufferSize,
	               const ParamObj& param)
	    requires Base::DataMetaBuffer && impl::overlay_parameterized<Base>
	    : Base(Impl(dataBufferPtr, dataBufferSize,
	                metaBufferPtr, metaBufferSize, param)) {}
	OverlayInlined(std::span<T> dataBuffer,
	               std::span<Meta> metaBuffer,
	               const ParamObj& param)
	    requires Base::DataMetaBuffer && impl::overlay_parameterized<Base>
	    : Base(Impl(dataBuffer.data(), dataBuffer.size(),
	                metaBuffer.data(), metaBuffer.size(), param)) {}

	// m_state will be default initialized into null state
	OverlayInlined() {}
};

// OverlayAlias's implementation
// do not touch
template <class OAlias>
class OverlayAliasImpl {
	friend OAlias;
	friend typename OAlias::OverlayBase;

private:
	using State = typename OAlias::State;

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
	using Base =
	    OverlayRelocationInterface<
	        OverlayInterface<
	            OverlayInternal<
	                Overlay<T, Impl, TArgs...>,
	                T>,
	            T>,
	        T>;
	using State = typename Base::State_impl;
	friend Impl;

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
    : public OverlayInterface<
          OverlayInternal<
              Overlay<T, OverlayInlinedImpl<OverlayMMW<Overlay, T, TArgs...>>, TArgs...>,
              T>,
          T> {
private:
	template <class... Args>
	static constexpr bool LastArgIsAlloc =
	    tx::type_list_count_v<tx::type_list_t<Args...>> &&
	    tx::allocator<tx::type_list_back_t<tx::type_list_t<Args...>>>;

	using InlinedImpl = OverlayInlinedImpl<OverlayMMW<Overlay, T, TArgs...>>;
	using Base = OverlayInterface<
	    OverlayInternal<
	        Overlay<T, InlinedImpl, TArgs...>,
	        T>,
	    T>;
	using State = typename Base::State_impl;
	using Allocator = std::conditional_t<
	    LastArgIsAlloc<TArgs...>,
	    tx::type_list_back_t<tx::type_list_t<TArgs...>>, std::allocator<T>>;
	using ParamObj = impl::overlay_parameter_object_t<Base>;
	friend InlinedImpl;

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
	    : Base(InlinedImpl(
	          alloc_traits<T>::allocate(alloc, bufferSize), bufferSize)),
	      m_alloc(std::move(alloc)) {}

	OverlayMMW(
	    u32 bufferSize, const ParamObj& param, Allocator alloc = Allocator{})
	    requires Base::SingleBuffer && impl::overlay_parameterized<Base>
	    : Base(InlinedImpl(
	          alloc_traits<T>::allocate(alloc, bufferSize), bufferSize, param)),
	      m_alloc(std::move(alloc)) {}

private:
};

} // namespace tx::impl