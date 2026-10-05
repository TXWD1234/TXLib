// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/allocator.hpp"
#include "impl/data_utils.hpp"
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
// > *It works, but it sucks. It sucks, but it works.*
// > *In this file, I suffer.* —— TXJerry

// ----------------------------------------------------------
// ················ Implementation Utilities ················
// ----------------------------------------------------------

template <class O>
concept overlay = true;
//requires(O o) {
// { o.valid() } -> std::same_as<bool>;
// o.destruct();
// typename O::StateStorage;

/**
	 * State_impl
	 * Parameters - optional
	 * 
	 * Internal Utility APIs
	 */
//};

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
	    OverlayBase*, T*, u32 expansionCount,
	    u32 currentSize, u32 currentCapacity) const {
		expand_impl(
		    expansionCount, currentSize, currentCapacity);
	}
	template <class OverlayBase, class T>
	void expandData(
	    OverlayBase*, T*, u32 expansionCount,
	    u32 currentSize, u32 currentCapacity) const {
		expand_impl(
		    expansionCount, currentSize, currentCapacity);
	}
	template <class OverlayBase, class T>
	void expandMeta(
	    OverlayBase*, T*, u32 expansionCount,
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
			            base->overlayRelocateBuffer(ptr, size);
		            });
	}

	void expandData(
	    OverlayBase* base, T* bufferPtr, u32 expansionCount,
	    u32 currentSize, u32 currentCapacity) const
	    requires OMMW::DataMetaBuffer
	{
		expand_impl(bufferPtr, expansionCount, currentSize, currentCapacity,
		            [base](T* ptr, u32 size) {
			            base->overlayRelocateBufferData(ptr, size);
		            });
	}
	void expandMeta(
	    OverlayBase* base, Meta* bufferPtr, u32 expansionCount,
	    u32 currentSize, u32 currentCapacity) const
	    requires OMMW::DataMetaBuffer
	{
		expand_impl(bufferPtr, expansionCount, currentSize, currentCapacity,
		            [base](T* ptr, u32 size) {
			            base->overlayRelocateBufferMeta(ptr, size);
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
	{ this->overlayRelocateBuffer(bufferPtr, bufferSize); }
	void relocate(std::span<T> buffer)
	    requires Base::SingleBuffer
	{ this->overlayRelocateBuffer(buffer.data(), buffer.size()); }

	// double buffer (data, meta)
	void relocate(T* dataBufferPtr, u32 dataBufferSize,
	              Meta* metaBufferPtr, u32 metaBufferSize)
	    requires Base::DataMetaBuffer
	{ this->overlayRelocateBuffer(
		dataBufferPtr, dataBufferSize,
		metaBufferPtr, metaBufferSize); }
	void relocate(std::span<T> dataBuffer,
	              std::span<Meta> metaBuffer)
	    requires Base::DataMetaBuffer
	{ this->overlayRelocateBuffer(
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
	OverlayInlined() : Base(Impl(), ExpansionHandler()) {}
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
	template <std::derived_from<typename Base::OverlayBase> O>
	OverlayAlias(O& parent)
	    : OverlayAlias(&parent.overlayGetStateProvider()()) {}

private:
	OverlayAlias(State* state)
	    : Base(Impl(state), ExpansionHandler{}) {}
};

// ################ MMW Overlay ################
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
	using Meta = typename Base::Meta;
	friend Impl;

private:
	template <class... Args>
	OverlayMMW(Allocator alloc, Args&&... args)
	    : Base(Impl(std::forward<Args>(args)...), ExpansionHandler(alloc)) {}

private:
	// ================ Allocation & Reallocation ================

	template <class U>
	using alloc_traits = tx::typed_allocator_traits<Allocator, U>;

	Allocator getAlloc_impl(const OverlayMMW* o) {
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
		this->overlayDestroyElements();
		this->overlayGetBufferState([this](T* ptr, u32 size) {
			alloc_traits<T>::deallocate(
			    getAlloc_impl(this), ptr, size);
		});
	}
	void destruct_impl()
	    requires Base::DataMetaBuffer
	{
		this->overlayDestroyElements();
		this->overlayGetBufferState(
		    [this](T* dataPtr, u32 dataSize, Base::Meta* metaPtr, u32 metaSize) {
			    auto& alloc = getAlloc_impl(this);
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
			this->overlaySetBufferState(
			    reallocate_impl(
			        getAlloc_impl(this), ptr, size,
			        other.overlayGetElementCount([](u32 osize) { return osize; })),
			    size); // size is updated to max(size, targetSize) in reallocate_impl
		});
	}
	void copyReallocate_impl(const OverlayMMW& other)
	    requires Base::DataMetaBuffer
	{
		this->overlayGetBufferState(
		    [this, &other](
		        T* dataPtr, u32 dataSize, Meta* metaPtr, u32 metaSize) {
			    this->overlaySetBufferState(
			        reallocate_impl(
			            getAlloc_impl(this), dataPtr, dataSize,
			            other.overlayGetElementCount([](u32 osize, u32) { return osize; })),
			        dataSize,
			        reallocate_impl(
			            getAlloc_impl(this), metaPtr, metaSize,
			            other.overlayGetElementCount([](u32, u32 osize) { return osize; })),
			        metaSize);
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

template <class T>
inline T* overlayNullCheck(
    T* ptr, u32 size,
    std::source_location loc = std::source_location::current()) {
	impl::assert_impl(assert::buffer_valid(ptr, size), loc);
	return ptr;
}

/**
 * Func will be called in constructor body. It's intended usage is to check
 * custom buffer input constraints, such as Pow2.
 * Buffer validity (null check) is already handled
 */
template <class T, std::invocable<T*&, u32&> Func>
struct OverlayBaseBufferStateSingle {
	OverlayBaseBufferStateSingle() = default;
	OverlayBaseBufferStateSingle(
	    T* dataPtr_, u32 dataSize_)
	    : ptr(overlayNullCheck(dataPtr_, dataSize_)),
	      size(dataSize_) {
		Func{}(ptr, size);
	}
	T* ptr = nullptr;
	u32 size = 0;

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
template <class T, class Meta, std::invocable<T*&, u32&, Meta*&, u32&> Func>
struct OverlayBaseBufferStateDataMeta {
	OverlayBaseBufferStateDataMeta() = default;
	OverlayBaseBufferStateDataMeta(
	    T* dataPtr_, u32 dataSize_, Meta* metaPtr_, u32 metaSize_)
	    : dataPtr(overlayNullCheck(dataPtr_, dataSize_)),
	      metaPtr(overlayNullCheck(metaPtr_, metaSize_)),
	      dataSize(dataSize_), metaSize(metaSize_) {
		Func{}(dataPtr, dataSize, metaPtr, metaSize);
	}
	T* dataPtr = nullptr;
	Meta* metaPtr = nullptr;
	u32 dataSize = 0;
	u32 metaSize = 0;

	void null_impl() {
		dataPtr = nullptr;
		metaPtr = nullptr;
		dataSize = 0;
		metaSize = 0;
	}

	// copy from
	void copy_impl(const OverlayBaseBufferStateDataMeta& other) {
		dataPtr = other.dataPtr;
		metaPtr = other.metaPtr;
		dataSize = other.dataSize;
		metaSize = other.metaSize;
	}

	OverlayBaseBufferStateDataMeta(const OverlayBaseBufferStateDataMeta&) = delete;
	OverlayBaseBufferStateDataMeta& operator=(const OverlayBaseBufferStateDataMeta&) = delete;
	OverlayBaseBufferStateDataMeta(OverlayBaseBufferStateDataMeta&& other)
	    : dataPtr(other.dataPtr), metaPtr(other.metaPtr),
	      dataSize(other.dataSize), metaSize(other.metaSize) { other.null_impl(); }
	OverlayBaseBufferStateDataMeta& operator=(OverlayBaseBufferStateDataMeta&& other) {
		if (&other == this) return *this;
		copy_impl(other);
		other.null_impl();
		return *this;
	};
};

} // namespace tx::impl