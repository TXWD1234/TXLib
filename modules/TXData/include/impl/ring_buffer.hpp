// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/data_foundation.hpp"
#include "impl/data_utils.hpp"
#include "impl/numeric_utils.hpp"
#include "impl/overlay.hpp"
#include "tx/exception.hpp"
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include <concepts>
#include <span>
#include <memory>
#include <format>

namespace tx {
/**
 * internal implementation of data structure, shouldn't be instantiated by user
 */
template <class T, template <class> class StateProviderTemplate, class BufferExpansionHandler>
class RingBufferOverlayBase {
protected:
	// default to null state
	struct State_impl {
		using BufferState_impl = impl::OverlayBaseBufferStateSingle<
		    T, decltype([](T*& ptr, u32& size) {
			    if (!tx::isPowTwo(size)) {
				    impl::assert_impl(
				        [] { return false; },
				        [=] {
					        return std::format(
					            "Bad construction. Argument `bufferSize` must"
					            " be a power of 2. bufferSize = {}",
					            size);
				        });
				    ptr = nullptr;
				    size = 0;
			    }
		    })>;
		BufferState_impl buffer;

		struct LogicState_impl {
			u32 begin = 0, end = 0;
		} logic;

		State_impl(T* ptr, u32 size) : buffer(ptr, size) {}
		State_impl() = default;
		// State_impl(T* ptr, u32 size, LogicalState logicState = LogicalState{}) : buffer(ptr, size), logic(logicState) {}
	};

	using StateProvider = StateProviderTemplate<State_impl>;
	friend StateProvider;
	friend BufferExpansionHandler;
	//static_assert(tx::invocable_r<StateProvider, State_impl&>);
	//static_assert(std::invocable<RingBufferOverlayBase, T*&, u32, u32, u32>);

public:
	using value_type = T;
	// DevNote: stale?
	using StateStorage = impl::Storage<State_impl>;

public:
	// DevNote: should this be move?
	RingBufferOverlayBase(StateProvider&& stateProvider,
	                      BufferExpansionHandler&& bufferExpansionHandler)
	    : m_state(std::move(stateProvider)),
	      m_expand(std::move(bufferExpansionHandler)) {}
	RingBufferOverlayBase() = default;

	RingBufferOverlayBase(const RingBufferOverlayBase&) = default;
	RingBufferOverlayBase& operator=(const RingBufferOverlayBase&) = default;
	RingBufferOverlayBase(RingBufferOverlayBase&& other) = default;
	RingBufferOverlayBase& operator=(RingBufferOverlayBase&& other) = default;

	bool valid() const { return state().buffer.ptr && state().buffer.size; }

public:
	// basic getter

	bool full() const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		return size() == state().buffer.size;
	}
	bool empty() const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		return state().logic.begin == state().logic.end;
	}
	u32 size() const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		return state().logic.end - state().logic.begin;
	}
	u32 capacity() const { return state().buffer.size; }
	T* data() { return state().buffer.ptr; }
	const T* data() const { return state().buffer.ptr; }

	// single operations

	template <class U>
	    requires std::is_constructible_v<T, U&&>
	void push_back(U&& val) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		expand_impl();
		std::construct_at(state().buffer.ptr + findPhysIndex_impl(state().logic.end), std::forward<U>(val));
		state().logic.end++;
	}
	template <class... Args>
	void emplace_back(Args&&... args) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		expand_impl();
		std::construct_at(state().buffer.ptr + findPhysIndex_impl(state().logic.end), std::forward<Args>(args)...);
		state().logic.end++;
	}

	void pop_back() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_empty(size()));
		state().logic.end--;
		std::destroy_at(state().buffer.ptr + findPhysIndex_impl(state().logic.end));
	}

	template <class U>
	    requires std::is_constructible_v<T, U&&>
	void push_front(U&& val) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		expand_impl();
		state().logic.begin--;
		std::construct_at(state().buffer.ptr + findPhysIndex_impl(state().logic.begin), std::forward<U>(val));
	}
	template <class... Args>
	void emplace_front(Args&&... args) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		expand_impl();
		state().logic.begin--;
		std::construct_at(state().buffer.ptr + findPhysIndex_impl(state().logic.begin), std::forward<Args>(args)...);
	}

	void pop_front() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_empty(size()));
		std::destroy_at(state().buffer.ptr + findPhysIndex_impl(state().logic.begin));
		state().logic.begin++;
	}

	// single data getters

	template <class Self>
	decltype(auto) front(this Self&& self) {
		impl::assert_impl(impl::assert::overlay_object_valid(&self));
		impl::assert_impl(impl::assert::buffer_not_empty(self.size()));
		tx::const_propagate<Self, T>* ptr = self.state().buffer.ptr;
		return *(ptr + self.findPhysIndex_impl(self.state().logic.begin));
	}
	template <class Self>
	decltype(auto) back(this Self&& self) {
		impl::assert_impl(impl::assert::overlay_object_valid(&self));
		impl::assert_impl(impl::assert::buffer_not_empty(self.size()));
		tx::const_propagate<Self, T>* ptr = self.state().buffer.ptr;
		return *(ptr + self.findPhysIndex_impl(self.state().logic.end - 1));
	}

	// multi and random access operations

	template <class Self>
	decltype(auto) operator[](this Self&& self, u32 index) {
		impl::assert_impl(impl::assert::overlay_object_valid(&self));
		impl::assert_impl(impl::assert::out_of_range(self.size(), index));
		tx::const_propagate<Self, T>* ptr = self.state().buffer.ptr;
		return *(ptr + self.findPhysIndex_impl(self.state().logic.begin + index));
	}

	void clear() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		if (empty()) return;
		u32 physBegin = findPhysIndex_impl(state().logic.begin);
		u32 physEnd = findPhysIndex_impl(state().logic.end);
		if (physBegin < physEnd) {
			// linear
			std::destroy(state().buffer.ptr + physBegin,
			             state().buffer.ptr + physEnd);
		} else if (physBegin > physEnd) {
			// wrap
			std::destroy(state().buffer.ptr,
			             state().buffer.ptr + physEnd);
			std::destroy(state().buffer.ptr + physBegin,
			             state().buffer.ptr + state().buffer.size);
		} else {
			// completely full
			// the empty edge case is prevented by the empty() check at the top
			std::destroy(state().buffer.ptr,
			             state().buffer.ptr + state().buffer.size);
		}
		state().logic.begin = 0;
		state().logic.end = 0;
	}

	// Flatten (unwrap) the ring buffer inplace
	void flatten() {
		// stub
	}
	// Flatten (unwrap) the ring buffer and copy it to another buffer
	// The provided destination buffer must have size bigger then the current
	// element count
	void flattenCopy(T* dest) const {
		flattenRelocate_impl(
		    [this](u32 first, u32 last, T* dest) {
			    std::uninitialized_copy(
			        this->state().buffer.ptr + first,
			        this->state().buffer.ptr + last,
			        dest);
		    },
		    dest, state().buffer.size, &state());
	}
	// Flatten (unwrap) the ring buffer and copy it to another buffer
	// The provided destination buffer must have size bigger then the current
	// element count
	// After this function's execution, all elements in this object's buffer
	// are in moved-from state. `clear()` is recommended to be called
	// immediately after.
	void flattenMove(T* dest) {
		flattenRelocate_impl(
		    [this](u32 first, u32 last, T* dest) {
			    std::uninitialized_move(
			        this->state().buffer.ptr + first,
			        this->state().buffer.ptr + last,
			        dest);
		    },
		    dest, state().buffer.size, &state());
	}

public:
	// lifetime APIs

	// <---------------------- remove
	// Destroies internal state object, ends lifetime of this overlay and every
	// other overlays that share the same buffers. Any other copies of this
	// overlay are now dangling and must not be used.
	// This is the point of no return.
	void destruct() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		std::destroy_at(m_state);
		// Not nulling the object because if so it would be inconsistent with
		// the alias objects of this object, since they are not nulled.
	}

private:
	// m_state is guaranteed to be valid, because this is only instantiated
	// internally.
	// But the State_impl object returned by `m_state()` might not be valid
	StateProvider m_state;
	BufferExpansionHandler m_expand;

private:
	// ================ Architectural Helpers ================

	// exist purely for intellisense to know the return type
	State_impl& state() { return m_state(); }

	void expand_impl(u32 expansionCount = 1) {
		m_expand(this, state().buffer.ptr, size(),
		         expansionCount, state().buffer.size);
	}

private:
	// ================ Logical Helpers ================

	// find physical index
	u32 findPhysIndex_impl(u32 index) const {
		return impl::findPowTwoWrappedPhysIndex(index, state().buffer.size);
	}

	template <std::invocable<u32, u32, T*> Func>
	static void flattenRelocate_impl(
	    Func&& copyFunc, T* dest, u32 dataBufferSize, const State_impl* state) {
		if (state->logic.begin == state->logic.end) return;
		u32 physBegin = impl::findPowTwoWrappedPhysIndex(state->logic.begin, dataBufferSize);
		u32 physEnd = impl::findPowTwoWrappedPhysIndex(state->logic.end, dataBufferSize);
		if (physBegin < physEnd) {
			// linear
			copyFunc(
			    physBegin,
			    physEnd,
			    dest);
		} else {
			// wrap
			copyFunc(
			    physBegin,
			    dataBufferSize,
			    dest);
			copyFunc(
			    0,
			    physEnd,
			    dest + (dataBufferSize - physBegin));
		}
	}

protected:
	/**
	 * List of intrinsics to be implemented:
	 * all functions taking in lambda should return whatever the lambda returns
	 * 
	 * [Exposure of Policy Object]
	 * - overlayGetStateProvider()
	 * - overlayGetBufferExpansionHandler()
	 * - overlaySetStateProvider() // support move
	 * - overlaySetBufferExpansionHandler() // support move
	 * 
	 * [BufferState management]
	 * - overlayGetBufferState(callback(T*, u32, [MetaT*, u32]))
	 * - overlaySetBufferState(BufferInfo) (SingleBuffer)
	 * - overlaySetBufferStateData(BufferInfo) (DataMetaBuffer)
	 * - overlaySetBufferStateMeta(BufferInfo) (DataMetaBuffer)
	 * - overlayRelocateBuffer(BufferInfo) (SingleBuffer)       // null guard internal
	 * - overlayRelocateBufferData(BufferInfo) (DataMetaBuffer) // null guard internal
	 * - overlayRelocateBufferMeta(BufferInfo) (DataMetaBuffer) // null guard internal
	 * 
	 * [LogicState & element management]
	 * - overlayGetElementCount(u32, [u32])
	 * - overlayCopyElements(T*, [Meta*]) // copy to
	 * - overlayGetLogicStateCopy() // specifically for MMW's state copy; return
	 *                              // manipulated LogicState_impl
	 * - overlaySetLogicState(LogicState_impl)
	 * 
	 * - overlayDestroyElements() // null guard
	 * 
	 * - using meta_type (DataMetaBuffer)
	 * //- using state_type
	 */







	// // destroy the current overlay object
	// // other copies of this overlay are not synced; it is not recommended to
	// // call this function on overlay that is not unique
	// void null_impl() {
	// 	state().buffer.ptr = nullptr;
	// 	m_state = nullptr;
	// 	state().buffer.size = 0;
	// }
	// void swap_impl(RingBufferOverlay<T>& other) {
	// 	std::swap(state().buffer.ptr, other.state().buffer.ptr);
	// 	std::swap(state().buffer.size, other.state().buffer.size);
	// 	std::swap(state().logic.begin, other.state().logic.begin);
	// 	std::swap(state().logic.end, other.state().logic.end);
	// }
	// // copy the data and state of another object after construction
	// // to fully sync with the other object
	// void copy_impl(const RingBufferOverlay<T>& other) {
	// 	state().logic.begin = other.state().logic.begin;
	// 	state().logic.end = other.state().logic.end;
	// 	if (other.empty()) return;
	// 	u32 physBegin = other.findPhysIndex_impl(other.state().logic.begin);
	// 	u32 physEnd = other.findPhysIndex_impl(other.state().logic.end);
	// 	if (physBegin < physEnd) {
	// 		// linear
	// 		std::uninitialized_copy(
	// 		    other.state().buffer.ptr + physBegin,
	// 		    other.state().buffer.ptr + physEnd,
	// 		    this->state().buffer.ptr + physBegin);
	// 	} else if (physBegin > physEnd) {
	// 		// wrap
	// 		std::uninitialized_copy(
	// 		    other.state().buffer.ptr,
	// 		    other.state().buffer.ptr + physEnd,
	// 		    this->state().buffer.ptr);
	// 		std::uninitialized_copy(
	// 		    other.state().buffer.ptr + physBegin,
	// 		    other.state().buffer.ptr + state().buffer.size,
	// 		    this->state().buffer.ptr + physBegin);
	// 	} else {
	// 		// completely full
	// 		// the empty edge case is prevented by the empty() check at the top
	// 		std::uninitialized_copy(
	// 		    other.state().buffer.ptr,
	// 		    other.state().buffer.ptr + state().buffer.size,
	// 		    this->state().buffer.ptr);
	// 	}
	// }
	// // query if object is valid
	// // used for defend moved-from object
	// bool isNull_impl() const {
	// 	return !state().buffer.ptr;
	// }
	// // called at destruction to clean up data
	// void destruct_impl() {
	// 	clear();
	// }
};

// template <class T, u32 size>
//     requires(size > 0 && (size & (size - 1)) == 0)
// class RingBuffer : public RingBufferOverlay<T> {
// public:
// 	RingBuffer()
// 	    : RingBufferOverlay<T>(
// 	          allocate<T>(size), size) {}
// 	~RingBuffer() {
// 		if (!this->isNull_impl()) {
// 			this->destruct_impl(); // destroy live elements before freeing
// 			free(this->data());
// 		}
// 	}

// 	RingBuffer(const RingBuffer<T, size>& other)
// 	    : RingBufferOverlay<T>(
// 	          allocate<T>(other.capacity()), other.capacity()) {
// 		copy_impl(other);
// 	}
// 	RingBuffer(RingBuffer<T, size>&& other) : RingBufferOverlay<T>(other) {
// 		// just use the copy constructor of RingBufferOverlay - shallow copy
// 		other.null_impl();
// 	}
// 	RingBuffer& operator=(RingBuffer<T, size> other) {
// 		this->swap_impl(other);
// 		return *this;
// 	}

// private:
// 	// cannot be swap_impl because ambiguity with base's swap_impl
// 	// this function exists for potential future expansion
// 	void swap(RingBuffer<T, size>& other) {
// 		swap_impl(other);
// 	}
// };
} // namespace tx