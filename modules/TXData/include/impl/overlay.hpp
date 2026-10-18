// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/allocator.hpp"
#include "impl/data_utils.hpp"
#include "impl/numeric_utils.hpp"
#include "tx/basic_types.hpp"
#include "tx/exception.hpp"
#include "tx/type_traits.hpp"
#include <memory>
#include <source_location>
#include <type_traits>
#include <concepts>
#include <span>


namespace tx::impl {
// ===========================================================
// **************** Overlay Pattern Utilities ****************
// ===========================================================
// > *Welcome to the darkest corner of TXLib.*
// > *It works, but it sucks. It sucks, but it works.*
// > *In this file, I suffer.* —— TXJerry

/**
 * Terminology:
 * True Overlay:  OverlayInlined and OverlayAlias, being completely non-owning,
 *                manage no memory, being the overlay state by the original
 *                Overlay Pattern.
 * MMW Overlay:   OverlayMMW and OverlayMMWAlias, own the data, menages memory,
 *                and handles reallocation. It is the vector-like high level
 *                dynamic buffer wrapper of the overlays.
 * Policy Object: The objects that OverlayBase takes, which is how wrappers
 *                change the behavior of OverlayBase.
 * State_impl:    The object that contain all data in OverlayBase, the only
 *                source of truth of an overlay data structure instance.
 * BufferState:   The buffer information stored in State_impl.
 */

// ----------------------------------------------------------
// ················ Implementation Utilities ················
// ----------------------------------------------------------

template <class O>
concept overlay = true;
//requires(O o) {
// { o.valid() } -> std::same_as<bool>;
// o.destruct();
// typename O::StateStorage;
// typename buffer_trait;

/**
	 * State_impl
	 * Parameters - optional
	 * 
	 * Internal Utility APIs
	 */
//};

// ================ Protected Accessor ================
// Grant access to OverlayBase's intrinsics for OverlayAlias, given that
// OverlayAlias need to call the intrinsics of any type of OverlayBase, which
// it might not be inheriting thereby has no privilege over that OverlayBase.

// The OverlayBase must
// `friend details::OverlayBaseIntrinsicsAccessor<OverlayBase>`

namespace details {
template <class O>
struct OverlayBaseIntrinsicsAccessor {
	static decltype(auto) overlayGetStateProvider(O& o) {
		return o.overlayGetStateProvider();
	}
};
} // namespace details

// ================ State Parameter Object ================

template <class O>
concept overlay_parameterized =
    overlay<O> &&
    requires {
	    typename O::Parameters;
    };

template <impl::overlay O>
using overlay_parameter_object_t = typename decltype([] {
	if constexpr (requires { typename O::Parameters; })
		return std::type_identity<typename O::Parameters>{};
	else
		return std::type_identity<tx::Nothing>{};
}())::type;

template <class T, class O>
concept overlay_parameter_object =
    impl::overlay_parameterized<O> &&
    std::same_as<T, impl::overlay_parameter_object_t<O>>;

// ================ Buffer Trait ================
/**
 * Defines the difference between varieties of buffer shapes, such as
 * SingleBuffer and DataMetaBuffer.
 * The OverlayBase class should define it's buffer trait via:
 * ```cpp
 * using buffer_trait = impl::overlay_single_buffer_trait<T, Policy>;
 * ```
 */

namespace details {
template <class Policy>
concept overlay_buffer_trait_realloc_policy =
    requires(u32 u) {
	    { Policy::expand(u, u, u) } -> std::convertible_to<u32>;
	    { Policy::copy(u) } -> std::convertible_to<u32>;
    };
} // namespace details

// ---------------- Presets ----------------

struct overlay_buffer_trait_realloc_policy_regular {
	static constexpr u32 ExpansionFactor = 2;
	static u32 expand(
	    u32 expansionCount, u32 currentSize, u32 currentCapacity) {
		return std::max(currentSize + expansionCount,
		                currentCapacity * ExpansionFactor);
	}
	static u32 copy(u32 size) { return size; }
};
struct overlay_buffer_trait_realloc_policy_pow_two {
	static constexpr u32 ExpansionFactor = 2;
	static u32 expand(
	    u32 expansionCount, u32 currentSize, u32 currentCapacity) {
		return tx::nextPowTwo(std::max(
		    currentSize + expansionCount,
		    currentCapacity * ExpansionFactor));
	}
	static u32 copy(u32 size) { return tx::nextPowTwo(size); }
};

template <class T,
          details::overlay_buffer_trait_realloc_policy Policy =
              overlay_buffer_trait_realloc_policy_regular>
struct overlay_single_buffer_trait : public Policy {
	using type = T;
	using realloc_policy = Policy;
};

template <class T, class Meta,
          details::overlay_buffer_trait_realloc_policy Policy =
              overlay_buffer_trait_realloc_policy_regular>
struct overlay_data_meta_buffer_trait : public Policy {
	using type = T;
	using meta_type = Meta;
	using realloc_policy = Policy;
};


namespace details {

// ---------------- Accessors ----------------
template <class Trait>
concept overlay_single_buffer =
    tx::instantiation_of<Trait, impl::overlay_single_buffer_trait>;
template <class Trait>
concept overlay_data_meta_buffer =
    tx::instantiation_of<Trait, impl::overlay_data_meta_buffer_trait>;


template <class Trait>
using overlay_data_t = typename Trait::type;

template <class Trait>
using overlay_meta_t = typename decltype([] {
	if constexpr (overlay_data_meta_buffer<Trait>)
		return std::type_identity<typename Trait::meta_type>{};
	else
		return std::type_identity<tx::Nothing>{};
}())::type;
} // namespace details

// ================ Buffer Expansion Handler ================
/**
 * Called when an insertion (eg. push_back, emplace_back, insert...) is
 * requested, and the logical size of the overlay base increments.
 * Also for the distinction of data expansion between True Overlay and MMW
 * Overlay, in which the latter dynamicly resize while the former asserts.
 */

/**
 * OverlayBase* base, T* data, u32 currentSize, u32 expansionCount, u32 currentCapacity
 */

namespace details {

// for True Overlay
struct OverlayBufferExpansionAssert {
private:
	void expand_impl(
	    u32 expansionCount, u32 currentSize, u32 currentCapacity) const {
		impl::assert_impl(assert::bad_expansion(
		    currentSize, currentCapacity, expansionCount));
	}

public:
	/**
	 * The extra `OverlayBase*` and `T*` parameter are just place holder to
	 * match up the signature of OverlayBufferExpansionResize
	 */

	template <class OverlayBase, class T>
	void expand(
	    OverlayBase*, u32 expansionCount, T*,
	    u32 currentSize, u32 currentCapacity) const {
		expand_impl(
		    expansionCount, currentSize, currentCapacity);
	}
	template <class OverlayBase, class T>
	void expandData(
	    OverlayBase*, u32 expansionCount, T*,
	    u32 currentSize, u32 currentCapacity) const {
		expand_impl(
		    expansionCount, currentSize, currentCapacity);
	}
	template <class OverlayBase, class Meta>
	void expandMeta(
	    OverlayBase*, u32 expansionCount, Meta*,
	    u32 currentSize, u32 currentCapacity) const {
		expand_impl(
		    expansionCount, currentSize, currentCapacity);
	}
};
// for MMW Overlay
/**
 * Because resizing need an allocator instance, as well as access to
 * OverlayBase's internal method `overlayRelocateBuffer`, this resize
 * class must become the master class of all MMW classes, and the Impl class of
 * OverlayMMW.
 * It handles resize, and stores the allocator. In construction of OverlayMMW,
 * allocation is done directly via the parameter allocator; In destruction of
 * OverlayMMW, deallocation is done via the allocator stored in this class,
 * which is acquired via overlayGetBufferExpansionHandler() from Base class
 */
template <class Allocator, class buffer_trait, class T>
struct OverlayBufferExpansionResize {
private:
	template <class U>
	using alloc_traits = tx::typed_allocator_traits<Allocator, U>;

private:
	template <class U, class Func>
	void expand_impl(
	    U* bufferPtr, u32 expansionCount,
	    u32 currentSize, u32 currentCapacity, Func&& f) const {
		if (currentSize + expansionCount <= currentCapacity) return;
		u32 newCapacity = buffer_trait::expand(
		    expansionCount, currentSize, currentCapacity);

		f(alloc_traits<U>::allocate(alloc, newCapacity),
		  newCapacity);
		alloc_traits<U>::deallocate(alloc, bufferPtr, currentCapacity);
	}

public:
	OverlayBufferExpansionResize(Allocator alloc_)
	    : alloc(std::move(alloc_)) {}
	OverlayBufferExpansionResize() = default;

	[[no_unique_address]] mutable Allocator alloc;

	template <class OverlayBase>
	void expand(
	    OverlayBase* base, u32 expansionCount, T* bufferPtr,
	    u32 currentSize, u32 currentCapacity) const
	    requires details::overlay_single_buffer<buffer_trait>
	{
		expand_impl(bufferPtr, expansionCount, currentSize, currentCapacity,
		            [base](T* ptr, u32 size) {
			            base->overlayRelocateBuffer(ptr, size);
		            });
	}

	template <class OverlayBase>
	void expandData(
	    OverlayBase* base, u32 expansionCount, T* bufferPtr,
	    u32 currentSize, u32 currentCapacity) const
	    requires details::overlay_data_meta_buffer<buffer_trait>
	{
		expand_impl(
		    bufferPtr, expansionCount, currentSize, currentCapacity,
		    [base](T* ptr, u32 size) {
			    base->overlayRelocateBufferData(ptr, size);
		    });
	}
	template <class OverlayBase>
	void expandMeta(
	    OverlayBase* base, u32 expansionCount,
	    details::overlay_meta_t<buffer_trait>* bufferPtr,
	    u32 currentSize, u32 currentCapacity) const
	    requires details::overlay_data_meta_buffer<buffer_trait>
	{
		expand_impl(
		    bufferPtr, expansionCount, currentSize, currentCapacity,
		    [base](details::overlay_meta_t<buffer_trait>* ptr, u32 size) {
			    base->overlayRelocateBufferMeta(ptr, size);
		    });
	}
};

struct OverlayBufferExpansionAssertTrait {
	template <class buffer_trait>
	using type = OverlayBufferExpansionAssert;
};
template <class Allocator, class T>
struct OverlayBufferExpansionResizeTrait {
	template <class buffer_trait>
	using type = OverlayBufferExpansionResize<Allocator, buffer_trait, T>;
};
} // namespace details

// ################ Base Subclass / Wrapper Utilities ################
/**
 * The sequence of applying the wrapper utility classes are non-randomizable.
 * OverlayInternal must be the first subclass around the raw Base class.
 * The rule is: From least specific / most generic (closer to the raw Base
 * class) to most specific / least generic (further from the raw Base class).
 */
namespace details {
/**
 * Basic subclass of OverlayBase
 * Providing addition to the protected internal utilities to serve all overlay
 * pattern wrappers
 */
template <impl::overlay O, class T>
class OverlayInternal : public O {
public:
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
	    details::overlay_single_buffer<typename O::buffer_trait>;
	static constexpr bool DataMetaBuffer =
	    details::overlay_data_meta_buffer<typename O::buffer_trait>;

protected:
	// ================ Buffer Trait Utilities ================

	using Meta = details::overlay_meta_t<typename O::buffer_trait>;

	static_assert(!SingleBuffer ||
	              (SingleBuffer &&
	               std::constructible_from<State, T*, u32>));
	static_assert(!DataMetaBuffer ||
	              (DataMetaBuffer &&
	               std::constructible_from<State, T*, u32, Meta*, u32>));


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
template <details::overlay_internal O, class T>
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
template <details::overlay_internal O, class T>
class OverlayRelocationInterface : public O {
private:
	using Base = O;
	using Meta_t = typename Base::Meta;

public:
	// ================ Relocation ================

	/**
	 * // DevNote: stub
	 * Rebind is stubbed for now. The architectural requirement of rebind is
	 * currently way too vague and cannot derive a stable interface. It will be
	 * completed when a demand appears
	 */

	// single buffer
	void relocate(T* bufferPtr, u32 bufferSize)
	    requires Base::SingleBuffer
	{ this->overlayRelocateBuffer(bufferPtr, bufferSize); }
	void relocate(std::span<T> buffer)
	    requires Base::SingleBuffer
	{ relocate(buffer.data(), buffer.size()); }

	// double buffer (data, meta)
	void relocate(T* dataBufferPtr, u32 dataBufferSize,
	              Meta_t* metaBufferPtr, u32 metaBufferSize)
	    requires Base::DataMetaBuffer
	{
		this->overlayRelocateBufferData(dataBufferPtr, dataBufferSize);
		this->overlayRelocateBufferMeta(metaBufferPtr, metaBufferSize);
	}
	void relocate(std::span<T> dataBuffer,
	              std::span<Meta_t> metaBuffer)
	    requires Base::DataMetaBuffer
	{ relocate(dataBuffer.data(), dataBuffer.size(),
		       metaBuffer.data(), metaBuffer.size()); }

	// ================ Buffer Information ================

	template <std::invocable<T*, u32> Func>
	void getBufferInfo(Func&& f)
	    requires Base::SingleBuffer
	{ this->overlayGetBufferState(std::forward<Func>(f)); }

	template <std::invocable<T*, u32, Meta_t*, u32> Func>
	void getBufferInfo(Func&& f)
	    requires Base::DataMetaBuffer
	{ this->overlayGetBufferState(std::forward<Func>(f)); }

public:
	using O::O;
};
} // namespace details

// ----------------------------------------------------------
// ················ Overlay Pattern Wrappers ················
// ----------------------------------------------------------
/**
 * # Conversion Hierarchy
 * Everything can be converted into OverlayAlias.
 * Only OverlayMMW can be converted into OverlayMMWAlias.
 * Nothing is convertable to OverlayInlined.
 * Nothing is convertable to OverlayMMW.
 */

/**
 * # The Impl Pattern
 * There are 2 essential policy functors taken by the OverlayBase:
 * - StateProvider: provide the State_impl object
 *   - variant: Owner (Inlined) / Alias
 * - BufferExpansionHandler: handle data expansion
 *   - variant: Assert (True Overlay) / Resize (MMW Overlay)
 * Some policy classes are direct implementation of some wrappers, namely
 * OverlayStateOwner -> OverlayInlined, OverlayStateAlias -> OverlayAlias,
 * OverlayBufferExpansionResize (OverlayMMWAllocationManager) -> OverlayMMW.
 * *They cannot be the class themselves is because reusability, and object
 * construction sequence.*
 */

// ################ True Overlay ################

namespace details {
// OverlayInlined's direct implementation
template <class State>
class OverlayStateOwner {
private:
	State m_state;

public:
	template <class... Args>
	    requires std::constructible_from<State, Args...>
	OverlayStateOwner(Args&&... args) : m_state(std::forward<Args>(args)...) {}

public:
	template <class Self>
	tx::const_propagate<Self, State>& operator()(this Self&& self) {
		return self.m_state;
	}
};

// OverlayAlias's direct implementation
template <class State>
class OverlayStateAlias {
private:
	State* m_state;

public:
	OverlayStateAlias(State* statePtr) : m_state(statePtr) {}

public:
	template <class Self>
	tx::const_propagate<Self, State>& operator()(this Self&& self) {
		return *static_cast<tx::const_propagate<Self, State>*>(self.m_state);
	}
};
} // namespace details

// Overlay Inlined
/**
 * The normal way of using an overlay.
 * The intended way to create an overlay object.
 */
template <template <class, template <class> class, class...> class Overlay, class T, class... TArgs>
// requires
class OverlayInlined
    : public details::OverlayRelocationInterface<
          details::OverlayInterface<
              details::OverlayInternal<
                  Overlay<T, details::OverlayStateOwner,
                          details::OverlayBufferExpansionAssertTrait, TArgs...>,
                  T>,
              T>,
          T> {
private:
	using ExpansionHandler = details::OverlayBufferExpansionAssert;
	using Base =
	    details::OverlayRelocationInterface<
	        details::OverlayInterface<
	            details::OverlayInternal<
	                Overlay<T, details::OverlayStateOwner,
	                        details::OverlayBufferExpansionAssertTrait, TArgs...>,
	                T>,
	            T>,
	        T>;
	using State = typename Base::State_impl;
	using Impl = details::OverlayStateOwner<State>;
	using ParamObj = impl::overlay_parameter_object_t<Base>;
	using Meta = typename Base::Meta;

private:
	OverlayInlined(Impl&& implObj)
	    : Base(std::move(implObj), ExpansionHandler{}) {}

public:
	// ================ Public Construction Interface ================

	// single buffer

	OverlayInlined(T* bufferPtr, u32 bufferSize)
	    requires Base::SingleBuffer
	    : OverlayInlined(Impl(bufferPtr, bufferSize)) {}
	OverlayInlined(std::span<T> buffer)
	    requires Base::SingleBuffer
	    : OverlayInlined(Impl(buffer.data(), buffer.size())) {}

	OverlayInlined(T* bufferPtr, u32 bufferSize, const ParamObj& param)
	    requires Base::SingleBuffer && impl::overlay_parameterized<Base>
	    : OverlayInlined(Impl(bufferPtr, bufferSize, param)) {}
	OverlayInlined(std::span<T> buffer, const ParamObj& param)
	    requires Base::SingleBuffer && impl::overlay_parameterized<Base>
	    : OverlayInlined(Impl(buffer.data(), buffer.size(), param)) {}

	// double buffer (data, meta)

	OverlayInlined(T* dataBufferPtr, u32 dataBufferSize,
	               Meta* metaBufferPtr, u32 metaBufferSize)
	    requires Base::DataMetaBuffer
	    : OverlayInlined(Impl(dataBufferPtr, dataBufferSize,
	                          metaBufferPtr, metaBufferSize)) {}
	OverlayInlined(std::span<T> dataBuffer,
	               std::span<Meta> metaBuffer)
	    requires Base::DataMetaBuffer
	    : OverlayInlined(Impl(dataBuffer.data(), dataBuffer.size(),
	                          metaBuffer.data(), metaBuffer.size())) {}

	OverlayInlined(T* dataBufferPtr, u32 dataBufferSize,
	               Meta* metaBufferPtr, u32 metaBufferSize,
	               const ParamObj& param)
	    requires Base::DataMetaBuffer && impl::overlay_parameterized<Base>
	    : OverlayInlined(Impl(dataBufferPtr, dataBufferSize,
	                          metaBufferPtr, metaBufferSize, param)) {}
	OverlayInlined(std::span<T> dataBuffer,
	               std::span<Meta> metaBuffer,
	               const ParamObj& param)
	    requires Base::DataMetaBuffer && impl::overlay_parameterized<Base>
	    : OverlayInlined(Impl(dataBuffer.data(), dataBuffer.size(),
	                          metaBuffer.data(), metaBuffer.size(), param)) {}

	// m_state will be default initialized into null state
	OverlayInlined() : Base(Impl(), ExpansionHandler()) {}
};

// Overlay Alias
/**
 * Handle object for concurrent access
 * Slightly slower then Inlined due to pointer indirection 
 */
template <template <class, template <class> class, class...> class Overlay, class T, class... TArgs>
// requires
class OverlayAlias
    : public details::OverlayRelocationInterface<
          details::OverlayInterface<
              details::OverlayInternal<
                  Overlay<T, details::OverlayStateAlias,
                          details::OverlayBufferExpansionAssertTrait, TArgs...>,
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
	using ExpansionHandler = details::OverlayBufferExpansionAssert;
	using Base =
	    details::OverlayRelocationInterface<
	        details::OverlayInterface<
	            details::OverlayInternal<
	                Overlay<T, details::OverlayStateAlias,
	                        details::OverlayBufferExpansionAssertTrait, TArgs...>,
	                T>,
	            T>,
	        T>;
	using State = typename Base::State_impl;
	using Impl = details::OverlayStateAlias<State>;

public:
	template <template <class, template <class> class, class...> class OTemplate,
	          template <class> class StateProviderTemplate,
	          class ExpansionHandlerTrait>
	OverlayAlias(
	    OTemplate<T, StateProviderTemplate, ExpansionHandlerTrait, TArgs...>& parent)
	    : OverlayAlias(
	          &details::OverlayBaseIntrinsicsAccessor<
	              OTemplate<T, StateProviderTemplate, ExpansionHandlerTrait, TArgs...>>::
	              overlayGetStateProvider(parent)()) {}

private:
	OverlayAlias(State* state)
	    : Base(Impl(state), ExpansionHandler{}) {}
};

// ################ MMW Overlay ################
// Memory Managing Wrapper

namespace details {
template <template <class, template <class> class, class...> class Overlay,
          class T, class... TArgs>
using ComposeOverlayBaseType =
    typename decltype(impl::ForwardLastAllocArgRemainder<TArgs...>::apply(
        []<class... Args>(tx::type_list_t<Args...>) {
	        return std::type_identity<Overlay<
	            T, details::OverlayStateOwner,
	            details::OverlayBufferExpansionResizeTrait<
	                impl::ParseLastAllocArg<std::allocator<T>, TArgs...>, T>,
	            Args...>>{};
        }))::type;
} // namespace details

/**
 * The high-level std::vector-like wrapper of overlays
 * This class manages memory and automatically resize when buffer is full
 * 
 * Template Parameters:
 * - `Allocator`: The last parameter of the template. The type does not matter.
 *                Default is std::allocator<T>
 */
template <template <class, template <class> class, class...> class Overlay, class T, class... TArgs>
class OverlayMMW
    : public details::OverlayInterface<
          details::OverlayInternal<
              details::ComposeOverlayBaseType<Overlay, T, TArgs...>,
              T>,
          T> {
private:
	using Allocator = impl::ParseLastAllocArg<std::allocator<T>, TArgs...>;
	using Base = details::OverlayInterface<
	    details::OverlayInternal<
	        details::ComposeOverlayBaseType<Overlay, T, TArgs...>, T>,
	    T>;
	using ExpansionHandler = details::OverlayBufferExpansionResize<
	    Allocator, typename Base::buffer_trait, T>;
	using State = typename Base::State_impl;
	using Impl = details::OverlayStateOwner<State>;
	using ParamObj = impl::overlay_parameter_object_t<Base>;
	using Meta = typename Base::Meta;
	using buffer_trait = typename Base::buffer_trait;

private:
	template <class... Args>
	OverlayMMW(Allocator alloc, Args&&... args)
	    : Base(Impl(std::forward<Args>(args)...), ExpansionHandler(alloc)) {}

private:
	// ================ Allocation & Reallocation ================

	template <class U>
	using alloc_traits = tx::typed_allocator_traits<Allocator, U>;

	static Allocator& getAlloc_impl(const OverlayMMW* o) {
		return o->overlayGetBufferExpansionHandler().alloc;
	}

public:
	// ================ Public Construction Interface ================

	OverlayMMW(
	    u32 bufferSize, Allocator alloc = Allocator{})
	    requires Base::SingleBuffer
	    : OverlayMMW(alloc,
	                 alloc_traits<T>::allocate(alloc, bufferSize),
	                 bufferSize) {}

	OverlayMMW(
	    u32 bufferSize, const ParamObj& param, Allocator alloc = Allocator{})
	    requires Base::SingleBuffer && impl::overlay_parameterized<Base>
	    : OverlayMMW(alloc,
	                 alloc_traits<T>::allocate(alloc, bufferSize),
	                 bufferSize, param) {}

	OverlayMMW(
	    u32 dataBufferSize, u32 metaBufferSize, Allocator alloc = Allocator{})
	    requires Base::DataMetaBuffer
	    : OverlayMMW(alloc,
	                 alloc_traits<T>::allocate(alloc, dataBufferSize),
	                 dataBufferSize,
	                 alloc_traits<T>::allocate(alloc, metaBufferSize),
	                 metaBufferSize) {}

	OverlayMMW(
	    u32 dataBufferSize, u32 metaBufferSize,
	    const ParamObj& param, Allocator alloc = Allocator{})
	    requires Base::DataMetaBuffer && impl::overlay_parameterized<Base>
	    : OverlayMMW(alloc,
	                 alloc_traits<T>::allocate(alloc, dataBufferSize),
	                 dataBufferSize,
	                 alloc_traits<T>::allocate(alloc, metaBufferSize),
	                 metaBufferSize, param) {}

	OverlayMMW() : Base(Impl(), ExpansionHandler()) {}

private:
	void destruct_impl()
	    requires Base::SingleBuffer
	{
		if (!this->valid()) return;
		this->overlayDestroyElements();
		this->overlayGetBufferState([this](T* ptr, u32 size) {
			alloc_traits<T>::deallocate(
			    getAlloc_impl(this), ptr, size);
		});
	}
	void destruct_impl()
	    requires Base::DataMetaBuffer
	{
		if (!this->valid()) return;
		this->overlayDestroyElements();
		this->overlayGetBufferState(
		    [this](T* dataPtr, u32 dataSize, Base::Meta* metaPtr, u32 metaSize) {
			    auto alloc = getAlloc_impl(this);
			    alloc_traits<T>::deallocate(alloc, dataPtr, dataSize);
			    alloc_traits<Meta>::deallocate(alloc, metaPtr, metaSize);
		    });
	}

	// elements at ptr must be already cleared
	template <class U>
	static U* reallocate_impl(Allocator alloc, U* ptr, u32& size, u32 targetSize) {
		if (targetSize > size) {
			alloc_traits<U>::deallocate(alloc, ptr, size);
			size = targetSize;
			return alloc_traits<U>::allocate(alloc, targetSize);
		}
		return ptr;
	}

	void copyReallocate_impl(const OverlayMMW& other)
	    requires Base::SingleBuffer
	{
		this->overlayGetBufferState([this, &other](T* ptr, u32 size) {
			T* newPtr = reallocate_impl(
			    getAlloc_impl(this), ptr, size,
			    buffer_trait::copy(other.overlayGetElementCount(
			        [](u32 osize) { return osize; })));
			this->overlaySetBufferState(
			    newPtr, size); // size is updated to max(size, targetSize) in
			// reallocate_impl
		});
	}
	void copyReallocate_impl(const OverlayMMW& other)
	    requires Base::DataMetaBuffer
	{
		this->overlayGetBufferState(
		    [this, &other](T* dataPtr, u32 dataSize, Meta* metaPtr, u32 metaSize) {
			    T* newDataPtr = reallocate_impl(
			        getAlloc_impl(this), dataPtr, dataSize,
			        buffer_trait::copy(other.overlayGetElementCount(
			            [](u32 osize, u32) { return osize; })));
			    this->overlaySetBufferStateData(
			        newDataPtr, dataSize);

			    Meta* newMetaPtr = reallocate_impl(
			        getAlloc_impl(this), metaPtr, metaSize,
			        buffer_trait::copy(other.overlayGetElementCount(
			            [](u32, u32 osize) { return osize; })));
			    this->overlaySetBufferStateMeta(
			        newMetaPtr, metaSize);
		    });
	}

	void copyCopyData_impl(const OverlayMMW& other)
	    requires Base::SingleBuffer
	{
		this->overlayGetBufferState(
		    [this, &other](
		        T* dataPtr, u32) {
			    other.overlayCopyElements(dataPtr);
		    });
	}
	void copyCopyData_impl(const OverlayMMW& other)
	    requires Base::DataMetaBuffer
	{
		this->overlayGetBufferState(
		    [this, &other](
		        T* dataPtr, u32, Meta* metaPtr, u32) {
			    other.overlayCopyElements(dataPtr, metaPtr);
		    });
	}

public:
	~OverlayMMW() { destruct_impl(); }


	OverlayMMW(const OverlayMMW& other)
	    requires Base::SingleBuffer
	    : Base(other.overlayGetBufferState([this, &other](T*, u32 size) {
		      return Impl(
		          alloc_traits<T>::allocate(getAlloc_impl(&other), size), size);
	      }),
	           ExpansionHandler(getAlloc_impl(&other))) {
		copyCopyData_impl(other);
		this->overlaySetLogicState(other.overlayGetLogicalStateCopy());
	}
	OverlayMMW(const OverlayMMW& other)
	    requires Base::DataMetaBuffer
	    : Base(other.overlayGetBufferState(
	               [this, &other](T*, u32 dataSize, Meta*, u32 metaSize) {
		               return Impl(
		                   alloc_traits<T>::allocate(
		                       getAlloc_impl(&other), dataSize),
		                   dataSize,
		                   alloc_traits<Meta>::allocate(
		                       getAlloc_impl(&other), metaSize),
		                   metaSize);
	               }),
	           ExpansionHandler(getAlloc_impl(&other))) {
		copyCopyData_impl(other);
		this->overlaySetLogicState(other.overlayGetLogicalStateCopy());
	}
	// copy does not propagate allocator
	OverlayMMW& operator=(const OverlayMMW& other) {
		if (&other == this) return *this;
		this->overlayDestroyElements();
		copyReallocate_impl(other);
		copyCopyData_impl(other);
		this->overlaySetLogicState(other.overlayGetLogicalStateCopy());
		return *this;
	}



	OverlayMMW(OverlayMMW&& other) = default;
	OverlayMMW& operator=(OverlayMMW&& other) {
		if (&other == this) return *this;
		this->destruct_impl();
		this->overlaySetStateProvider(
		    std::move(other.overlayGetStateProvider()));
		this->overlaySetBufferExpansionHandler(
		    std::move(other.overlayGetBufferExpansionHandler()));
		return *this;
	};
};

// ################ Overlay Base Implementation Utilities ################
/**
 * Overlay Base Template Parameters:
 * template <class T,
          template <class> class StateProviderTemplate,
          class BufferExpansionHandlerTraits, class...>
 */

template <class T>
inline T* overlayNullCheck(
    T* ptr, u32 size,
    std::source_location loc = std::source_location::current()) {
	impl::assert_impl(assert::buffer_valid(ptr, size), loc);
	return ptr;
}

// ================ Buffer State ================
/**
 * Specification:
 * - Everything public
 * - Template Param:
 *   - must have `std::invocable<BufferInfo> Func = tx::Nothing`
 * - Construction:
 *   - must exist:
 *    - default constructor which nulls everything
 *    - constructor taking buffer info (standard constructor)
 *     - call `Func` in constructor body
 *    - move constructor
 *   - must not exist:
 *    - copy semantics
 * - Member:
 *   - `valid()` checking null state
 *   - `null_impl()` nulling self
 *   - `copy_impl(other)` copy from other to self
 */
/**
 * Usage:
 * Second Parameter Argument: Func will be called in constructor body. It's
 * intended usage is to check custom buffer input constraints, such as Pow2.
 * Buffer validity (null check) is already handled.
 */


template <class T, std::invocable<T*&, u32&> Func = tx::Nothing>
struct OverlayBaseBufferStateSingle {
	OverlayBaseBufferStateSingle() = default;
	OverlayBaseBufferStateSingle(
	    T* dataPtr_, u32 dataSize_, Func f = Func{})
	    : ptr(overlayNullCheck(dataPtr_, dataSize_)),
	      size(dataSize_) {
		f(ptr, size);
	}
	T* ptr = nullptr;
	u32 size = 0;

	bool valid() const { return ptr && size; }

	void null_impl() {
		ptr = nullptr;
		size = 0;
	}

	// copy from
	void copy_impl(const OverlayBaseBufferStateSingle& other) {
		ptr = other.ptr;
		size = other.size;
	}

	OverlayBaseBufferStateSingle(const OverlayBaseBufferStateSingle&) = delete;
	OverlayBaseBufferStateSingle& operator=(const OverlayBaseBufferStateSingle&) = delete;
	OverlayBaseBufferStateSingle(OverlayBaseBufferStateSingle&& other)
	    : ptr(other.ptr), size(other.size) { other.null_impl(); }
	OverlayBaseBufferStateSingle& operator=(OverlayBaseBufferStateSingle&& other) {
		if (&other == this) return *this;
		copy_impl(other);
		other.null_impl();
		return *this;
	};
};

/**
 * Func will be called in constructor body. It's intended usage is to check
 * custom buffer input constraints, such as Pow2.
 * Buffer validity (null check) is already handled
 */
template <class T, class Meta,
          std::invocable<T*&, u32&> DataFunc = tx::Nothing,
          std::invocable<Meta*&, u32&> MetaFunc = tx::Nothing>
struct OverlayBaseBufferStateDataMeta {
	OverlayBaseBufferStateDataMeta() = default;
	OverlayBaseBufferStateDataMeta(
	    T* dataPtr_, u32 dataSize_,
	    Meta* metaPtr_, u32 metaSize_,
	    DataFunc df = DataFunc{}, MetaFunc mf = MetaFunc{})
	    : data(dataPtr_, dataSize_, df),
	      meta(metaPtr_, metaSize_, mf) {}
	impl::OverlayBaseBufferStateSingle<T, DataFunc> data;
	impl::OverlayBaseBufferStateSingle<Meta, MetaFunc> meta;

	bool valid() const { return data.valid() && meta.valid(); }

	void null_impl() {
		data.null_impl();
		meta.null_impl();
	}

	// copy from
	void copy_impl(const OverlayBaseBufferStateDataMeta& other) {
		data.copy_impl(other.data);
		meta.copy_impl(other.meta);
	}

	OverlayBaseBufferStateDataMeta(const OverlayBaseBufferStateDataMeta&) = delete;
	OverlayBaseBufferStateDataMeta& operator=(const OverlayBaseBufferStateDataMeta&) = delete;
	OverlayBaseBufferStateDataMeta(OverlayBaseBufferStateDataMeta&& other)
	    : data(std::move(other.data)), meta(std::move(other.meta)) { other.null_impl(); }
	OverlayBaseBufferStateDataMeta& operator=(OverlayBaseBufferStateDataMeta&& other) {
		if (&other == this) return *this;
		copy_impl(other);
		other.null_impl();
		return *this;
	};
};

// ================ CRTP Interface Helpers ================
/**
 * Interface Helpers classes inherited by OverlayBase must be friended by
 * OverlayBase.
 */


// Generalizing the internal implementation and provide helpers for OverlayBase
// Should be inherited privately.
template <impl::overlay OverlayBase, class buffer_trait>
struct OverlayBaseInternal {
private:
	static constexpr bool SingleBuffer =
	    details::overlay_single_buffer<buffer_trait>;
	static constexpr bool DataMetaBuffer =
	    details::overlay_data_meta_buffer<buffer_trait>;

private:
	template <class Self>
	tx::const_propagate<Self, OverlayBase>* self(this Self&& s) {
		return static_cast<tx::const_propagate<Self, OverlayBase>*>(&s);
	}
	template <class Self>
	decltype(auto) state(this Self&& s) { return s.self()->m_state(); }

public:
	// @param size logical size
	void expand(u32 size, u32 expansionCount = 1)
	    requires SingleBuffer
	{
		self()->m_expand.expand(
		    self(), expansionCount,
		    state().buffer.ptr, size, state().buffer.size);
	}
	// @param size logical size
	void expandData(u32 size, u32 expansionCount = 1)
	    requires DataMetaBuffer
	{
		self()->m_expand.expandData(
		    self(), expansionCount,
		    state().buffer.data.ptr, size, state().buffer.data.size);
	}
	// @param size logical size
	void expandMeta(u32 size, u32 expansionCount = 1)
	    requires DataMetaBuffer
	{
		self()->m_expand.expandMeta(
		    self(), expansionCount,
		    state().buffer.meta.ptr, size, state().buffer.meta.size);
	}
};

// Provide public interface for OverlayBase
// *CRTP finally being used like CRTP.*
// Should be inherited publicly
template <impl::overlay OverlayBase>
struct OverlayBaseInterface {
private:
	template <class Self>
	tx::const_propagate<Self, OverlayBase>* self(this Self&& s) {
		return static_cast<tx::const_propagate<Self, OverlayBase>*>(&s);
	}

public:
	// ---------------- Overlay Pattern Oriented ----------------

	bool valid() const { return self()->m_state().buffer.valid(); }

public:
	// ---------------- General Public Interface ----------------

	/**
	 * size/empty/full/capacity/data
	 */
};

// Provide default intrinsic interface for OverlayBase
// Should be inherited protectedly.
/**
 * This Intrinsic class only provide default intrinsics created under
 * general assumptions of normal overlay data structures. If any specific
 * custom conditions apply, shadow the functions with specific logic in the
 * OverlayBase implementation.
 */
template <impl::overlay OverlayBase, class State,
          class StateProvider, class BufferExpansionHandler,
          class buffer_trait>
struct OverlayBaseIntrinsics {
	/**
	 * All default intrinsic implementation are encouraged to use as few
	 * internal access privilege as possible, and replace direct internal
	 * access with intrinsics as much as possible.
	 * The reason for that is because all intrinsics imeplementation are
	 * implemented under assumption, and might be overridden by the OverlayBase
	 * class implementation. When an intrinsic implementation calls other
	 * intrinsic instead of hardcoded default logic, the called intrinsic is
	 * guaranteed to contain correct logic.
	 */

private:
	static constexpr bool SingleBuffer =
	    details::overlay_single_buffer<buffer_trait>;
	static constexpr bool DataMetaBuffer =
	    details::overlay_data_meta_buffer<buffer_trait>;

	using T = details::overlay_data_t<buffer_trait>;
	using Meta = details::overlay_meta_t<buffer_trait>;

private:
	template <class Self>
	tx::const_propagate<Self, OverlayBase>* self(this Self&& s) {
		return static_cast<tx::const_propagate<Self, OverlayBase>*>(&s);
	}
	template <class Self>
	decltype(auto) state(this Self&& s) { return s.self()->m_state(); }

protected:
	// ---------------- Policy Object Exposure ----------------

	template <class Self>
	tx::const_propagate<Self, StateProvider>&
	overlayGetStateProvider(this Self&& s) {
		return static_cast<tx::const_propagate<Self, OverlayBase>*>(&s)
		    ->m_state;
	}
	template <class Self>
	tx::const_propagate<Self, BufferExpansionHandler>&
	overlayGetBufferExpansionHandler(this Self&& s) {
		return static_cast<tx::const_propagate<Self, OverlayBase>*>(&s)
		    ->m_expand;
	}

	template <class U>
	    requires std::same_as<
	        std::remove_cvref_t<U>, typename OverlayBase::StateProvider>
	void overlaySetStateProvider(U&& stateProvider) {
		self()->m_state = std::forward<U>(stateProvider);
	}
	template <class U>
	    requires std::same_as<std::remove_cvref_t<U>, BufferExpansionHandler>
	void overlaySetBufferExpansionHandler(U&& bufferExpansionHandler) {
		self()->m_expand = std::forward<U>(bufferExpansionHandler);
	}

protected:
	// ---------------- Buffer State Management ----------------

	template <class Func>
	    requires((SingleBuffer && std::invocable<Func, T*, u32>) ||
	             (DataMetaBuffer && std::invocable<Func, T*, u32, Meta*, u32>))
	decltype(auto) overlayGetBufferState(Func&& f) const {
		if constexpr (SingleBuffer) {
			return f(state().buffer.ptr, state().buffer.size);
		} else if constexpr (DataMetaBuffer) {
			return f(
			    state().buffer.data.ptr, state().buffer.data.size,
			    state().buffer.meta.ptr, state().buffer.meta.size);
		}
	}

	void overlaySetBufferState(T* ptr, u32 size)
	    requires SingleBuffer
	{ impl::reconstruct(state().buffer, ptr, size); }
	void overlaySetBufferStateData(T* dataPtr, u32 dataSize)
	    requires DataMetaBuffer
	{ impl::reconstruct(state().buffer.data, dataPtr, dataSize); }
	void overlaySetBufferStateMeta(Meta* metaPtr, u32 metaSize)
	    requires DataMetaBuffer
	{ impl::reconstruct(state().buffer.meta, metaPtr, metaSize); }

	/**
	 * The default relocation is a plain memcpy for all buffers.
	 * It is encouraged and common to override the relocation functions if any
	 * custom logic apart from plain memcpy is present.
	 */

	void overlayRelocateBuffer(T* ptr, u32 size)
	    requires SingleBuffer
	{
		self()->overlayGetBufferState([this, ptr, size](T* oPtr, u32 oSize) {
			self()->overlaySetBufferState(ptr, size);
			tx::uninitialized_relocate(oPtr, oPtr + oSize, ptr);
		});
	}
	void overlayRelocateBufferData(T* dataPtr, u32 dataSize)
	    requires DataMetaBuffer
	{
		self()->overlaySetBufferStateData(dataPtr, dataSize);
	}
	void overlayRelocateBufferMeta(Meta* metaPtr, u32 metaSize)
	    requires DataMetaBuffer
	{

		self()->overlaySetBufferStateMeta(metaPtr, metaSize);
	}
};
} // namespace tx::impl

/**
 * TODO:
 * [Minor]
 * - refactor all state references via inheritance to the independent type
 * - add concepts where ever fits
 */