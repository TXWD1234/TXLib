// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/allocator.hpp"
#include "impl/data_utils.hpp"
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include <memory>
#include <type_traits>
#include <concepts>
#include <span>


namespace tx::impl {
// ===========================================================
// **************** Overlay Pattern Utilities ****************
// ===========================================================
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

// ################ Implementation Utilities ################

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
		return std::type_identity<void>{};
}())::type;

template <class T, class O>
concept overlay_parameter_object =
    impl::overlay_parameterized<O> &&
    std::same_as<T, impl::overlay_parameter_object_t<O>>;

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
		// <-------------------- bad expansion
	}

public:
	/**
	 * The extra `base` and `bufferPtr` parameter are just place holder to
	 * match up the signature of OverlayBufferExpansionResize
	 */

	template <class OverlayBase, class T>
	void expand(
	    OverlayBase* base, T* bufferPtr, u32 expansionCount,
	    u32 currentSize, u32 currentCapacity) const {
		expand_impl(
		    expansionCount, currentSize, currentCapacity);
	}
	template <class OverlayBase, class T>
	void expandData(
	    OverlayBase* base, T* bufferPtr, u32 expansionCount,
	    u32 currentSize, u32 currentCapacity) const {
		expand_impl(
		    expansionCount, currentSize, currentCapacity);
	}
	template <class OverlayBase, class T>
	void expandMeta(
	    OverlayBase* base, T* bufferPtr, u32 expansionCount,
	    u32 currentSize, u32 currentCapacity) const {
		expand_impl(
		    expansionCount, currentSize, currentCapacity);
	}
};
// for MMW Overlay
/**
 * Because resizing need an allocator instance, as well as access to
 * OverlayBase's internal method `overlayRelocateBufferState_impl`, this resize
 * class must become the master class of all MMW classes, and the Impl class of
 * OverlayMMW.
 * It handles resize, and stores the allocator. In construction of OverlayMMW,
 * allocation is done directly via the parameter allocator; In destruction of
 * OverlayMMW, deallocation is done via the allocator stored in this class,
 * which is acquired via overlayGetBufferExpansionHandler_impl() from Base class
 */
template <class OMMW>
struct OverlayBufferExpansionResize {
private:
	using Allocator = typename OMMW::Allocator;
	using Meta = typename OMMW::Meta;
	using OverlayBase = typename OMMW::OverlayBase;
	using T = typename OMMW::value_type;
	template <class U>
	using alloc_traits = tx::typed_allocator_traits<Allocator, U>;

private:
	static constexpr u32 ExpansionFactor = 2;
	u32 findNewCapacity_impl(
	    u32 expansionCount, u32 currentSize, u32 currentCapacity) {
		return std::max(currentSize + expansionCount,
		                currentCapacity * ExpansionFactor);
	}

	template <class Func>
	void expand_impl(
	    T* bufferPtr, u32 expansionCount,
	    u32 currentSize, u32 currentCapacity, Func&& f) const {
		if (currentSize + expansionCount <= currentCapacity) return;
		u32 newCapacity = findNewCapacity_impl(
		    expansionCount, currentSize, currentCapacity);

		f(alloc_traits<T>::allocate(alloc, newCapacity),
		  newCapacity);
		alloc_traits<T>::deallocate(alloc, bufferPtr, currentCapacity);
	}

public:
	OverlayBufferExpansionResize(Allocator alloc_)
	    : alloc(std::move(alloc_)) {}
	OverlayBufferExpansionResize() = default;

	[[no_unique_address]] mutable Allocator alloc;


	void expand(
	    OverlayBase* base, T* bufferPtr, u32 expansionCount,
	    u32 currentSize, u32 currentCapacity) const
	    requires OMMW::SingleBuffer
	{
		expand_impl(bufferPtr, expansionCount, currentSize, currentCapacity,
		            [base](T* ptr, u32 size) {
			            base->overlayRelocateBufferState(ptr, size);
		            });
	}

	void expandData(
	    OverlayBase* base, T* bufferPtr, u32 expansionCount,
	    u32 currentSize, u32 currentCapacity) const
	    requires OMMW::DataMetaBuffer
	{
		expand_impl(bufferPtr, expansionCount, currentSize, currentCapacity,
		            [base](T* ptr, u32 size) {
			            base->overlayRelocateBufferStateData(ptr, size);
		            });
	}
	void expandMeta(
	    OverlayBase* base, Meta* bufferPtr, u32 expansionCount,
	    u32 currentSize, u32 currentCapacity) const
	    requires OMMW::DataMetaBuffer
	{
		expand_impl(bufferPtr, expansionCount, currentSize, currentCapacity,
		            [base](T* ptr, u32 size) {
			            base->overlayRelocateBufferStateMeta(ptr, size);
		            });
	}
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
	    std::constructible_from<
	        State, T*, u32>;
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
	using Meta = typename Base::Meta;

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
	{ this->overlayRelocateBufferState(bufferPtr, bufferSize); }
	void relocate(std::span<T> buffer)
	    requires Base::SingleBuffer
	{ this->overlayRelocateBufferState(buffer.data(), buffer.size()); }

	// double buffer (data, meta)
	void relocate(T* dataBufferPtr, u32 dataBufferSize,
	              Meta* metaBufferPtr, u32 metaBufferSize)
	    requires Base::DataMetaBuffer
	{ this->overlayRelocateBufferState(
		dataBufferPtr, dataBufferSize,
		metaBufferPtr, metaBufferSize); }
	void relocate(std::span<T> dataBuffer,
	              std::span<Meta> metaBuffer)
	    requires Base::DataMetaBuffer
	{ this->overlayRelocateBufferState(
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
} // namespace details

// ################ Overlay Pattern Wrappers ################
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

namespace details {
// OverlayInlined's direct implementation
template <class OInlined>
class OverlayStateOwner {
	friend OInlined;

private:
	using State = typename OInlined::State;

private:
	State m_state;

private:
	template <class... Args>
	OverlayStateOwner(Args&&... args) : m_state(std::forward<Args>(args)...) {}

public:
	template <class Self>
	tx::const_propagate<Self, State>& operator()(this Self&& self) {
		return self.m_state;
	}

	OverlayStateOwner(OverlayStateOwner&&) = default;
};

// OverlayAlias's direct implementation
template <class OAlias>
class OverlayStateAlias {
	friend OAlias;

private:
	using State = typename OAlias::State;

private:
	State* m_state;

private:
	OverlayStateAlias(State* statePtr) : m_state(statePtr) {}

public:
	template <class Self>
	tx::const_propagate<Self, State>& operator()(this Self&& self) {
		return *static_cast<tx::const_propagate<Self, State>*>(self.m_state);
	}

	OverlayStateAlias(OverlayStateAlias&&) = default;
};
} // namespace details

// Overlay Inlined
/**
 * The normal way of using an overlay.
 * The intended way to create an overlay object.
 */
template <template <class...> class Overlay, class T, class... TArgs>
// requires
class OverlayInlined
    : public details::OverlayRelocationInterface<
          details::OverlayInterface<
              details::OverlayInternal<
                  Overlay<T, details::OverlayStateOwner<OverlayInlined<Overlay, T, TArgs...>>,
                          details::OverlayBufferExpansionAssert, TArgs...>,
                  T>,
              T>,
          T> {
private:
	using Impl = details::OverlayStateOwner<OverlayInlined>;
	using ExpansionHandler = details::OverlayBufferExpansionAssert;
	using Base =
	    details::OverlayRelocationInterface<
	        details::OverlayInterface<
	            details::OverlayInternal<
	                Overlay<T, Impl, ExpansionHandler, TArgs...>,
	                T>,
	            T>,
	        T>;
	using State = typename Base::State_impl;
	using ParamObj = impl::overlay_parameter_object_t<Base>;
	using Meta = typename Base::Meta;
	friend Impl;

private:
	template <class... Args>
	OverlayInlined(Args&&... args)
	    : Base(Impl(std::forward<Args>(args)...), ExpansionHandler{}) {}
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
	    : OverlayInlined(buffer.data(), buffer.size()) {}

	OverlayInlined(T* bufferPtr, u32 bufferSize, const ParamObj& param)
	    requires Base::SingleBuffer && impl::overlay_parameterized<Base>
	    : OverlayInlined(Impl(bufferPtr, bufferSize, param)) {}
	OverlayInlined(std::span<T> buffer, const ParamObj& param)
	    requires Base::SingleBuffer && impl::overlay_parameterized<Base>
	    : OverlayInlined(buffer.data(), buffer.size(), param) {}

	// double buffer (data, meta)

	OverlayInlined(T* dataBufferPtr, u32 dataBufferSize,
	               Meta* metaBufferPtr, u32 metaBufferSize)
	    requires Base::DataMetaBuffer
	    : OverlayInlined(Impl(dataBufferPtr, dataBufferSize,
	                          metaBufferPtr, metaBufferSize)) {}
	OverlayInlined(std::span<T> dataBuffer,
	               std::span<Meta> metaBuffer)
	    requires Base::DataMetaBuffer
	    : OverlayInlined(dataBuffer.data(), dataBuffer.size(),
	                     metaBuffer.data(), metaBuffer.size()) {}

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
	    : OverlayInlined(dataBuffer.data(), dataBuffer.size(),
	                     metaBuffer.data(), metaBuffer.size(), param) {}

	// m_state will be default initialized into null state
	OverlayInlined() : Base() {}
};

// Overlay Alias
/**
 * Handle object for concurrent access
 * Slightly slower then Inlined due to pointer indirection 
 */
template <template <class...> class Overlay, class T, class... TArgs>
// requires
class OverlayAlias
    : public details::OverlayRelocationInterface<
          details::OverlayInterface<
              details::OverlayInternal<
                  Overlay<T, details::OverlayStateAlias<OverlayAlias<Overlay, T, TArgs...>>,
                          details::OverlayBufferExpansionAssert, TArgs...>,
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
	using Impl = details::OverlayStateAlias<OverlayAlias>;
	using ExpansionHandler = details::OverlayBufferExpansionAssert;
	using Base =
	    details::OverlayRelocationInterface<
	        details::OverlayInterface<
	            details::OverlayInternal<
	                Overlay<T, Impl, ExpansionHandler, TArgs...>,
	                T>,
	            T>,
	        T>;
	using State = typename Base::State_impl;
	friend Impl;

public:
	// <------------------------------------------------------------------ accept any overlay
	OverlayAlias(OverlayInlined<Overlay, T, TArgs...>& parent)
	    : OverlayAlias(&parent.overlayGetStateProvider_impl()()) {}

private:
	OverlayAlias(State* state)
	    : Base(Impl(state), ExpansionHandler{}) {}
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
    : public details::OverlayInterface<
          details::OverlayInternal<
              Overlay<T, details::OverlayStateOwner<OverlayMMW<Overlay, T, TArgs...>>,
                      details::OverlayBufferExpansionResize<
                          OverlayMMW<Overlay, T, TArgs...>>,
                      TArgs...>,
              T>,
          T> {
private:
	template <class... Args>
	static constexpr bool LastArgIsAlloc =
	    tx::type_list_count_v<tx::type_list_t<Args...>> &&
	    tx::allocator<tx::type_list_back_t<tx::type_list_t<Args...>>>;

	using Impl = details::OverlayStateOwner<OverlayMMW<Overlay, T, TArgs...>>;
	using ExpansionHandler = details::OverlayBufferExpansionResize<OverlayMMW>;
	using Base = details::OverlayInterface<
	    details::OverlayInternal<
	        Overlay<T, Impl, ExpansionHandler, TArgs...>,
	        T>,
	    T>;
	using State = typename Base::State_impl;
	using Allocator = std::conditional_t<
	    LastArgIsAlloc<TArgs...>,
	    tx::type_list_back_t<tx::type_list_t<TArgs...>>, std::allocator<T>>;
	using ParamObj = impl::overlay_parameter_object_t<Base>;
	friend Impl;

private:
	template <class... Args>
	OverlayMMW(Allocator alloc, Args&&... args)
	    : Base(Impl(std::forward<Args>(args)...), ExpansionHandler(alloc)) {}

private:
	// ================ Allocation & Reallocation ================

	template <class U>
	using alloc_traits = tx::typed_allocator_traits<Allocator, U>;

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

public:
	~OverlayMMW()
	    requires Base::SingleBuffer
	{
		this->overlayDestroyElements();
		this->overlayGetBufferState([this](T* ptr, u32 size) {
			alloc_traits<T>::deallocate(
			    this->overlayGetBufferExpansionHandler().alloc,
			    ptr, size);
		});
	}
	~OverlayMMW()
	    requires Base::DataMetaBuffer
	{
		this->overlayDestroyElements();
		this->overlayGetBufferState(
		    [this](T* dataPtr, u32 dataSize, Base::Meta* metaPtr, u32 metaSize) {
			    auto& alloc = this->overlayGetBufferExpansionHandler().alloc;
			    alloc_traits<T>::deallocate(alloc, dataPtr, dataSize);
			    alloc_traits<T>::deallocate(alloc, metaPtr, metaSize);
		    });
	}
};

// ################ Overlay Base Implementation Utilities ################

template <class T>
inline void overlayNullCheck(T* ptr, u32 size) {
	// <------------------------------------------------- assert_impl
}

} // namespace tx::impl