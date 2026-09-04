// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/data_utils.hpp"
#include "impl/numeric_utils.hpp"
#include "tx/exception.hpp"
#include "tx/basic_types.hpp"
#include <span>
#include <memory>
#include <format>

namespace tx {
/**
 * Every time before you push, you should check `full()`.
 *   Any push operation on an full buffer is Undefined Behavior
 * Every time before you pop, you should check `empty()`.
 *   Any pop operation on an empty buffer is Undefined Behavior
 */
template <class T>
class RingBufferOverlay {
private:
	struct State_impl {
		u32 begin = 0, end = 0;
	};

public:
	using value_type = T;
	using StateStorage = impl::Storage<State_impl>;

public:
	/**
	 * @param ptr the data pointer to a piece of memory that has at least
	 * size of `capacity`.
	 * @param size the capacity of this container object. It cannot resize.
	 * It must be a power of 2. If not, the object created by it will be
	 * invalid
	 */
	RingBufferOverlay(T* bufferPtr, u32 bufferSize, StateStorage* statePtr)
	    : m_data(tx::isPowTwo(bufferSize) ? bufferPtr : nullptr),
	      m_state(std::construct_at(reinterpret_cast<State_impl*>(statePtr))),
	      m_size(tx::isPowTwo(bufferSize) ? bufferSize : 0) {
		impl::assert_impl(
		    [&] { return valid(); },
		    [&] {
			    if (!tx::isPowTwo(bufferSize)) {
				    return std::format(
				        "Bad construction. Argument `bufferSize` must be a power of 2. bufferSize = {}",
				        bufferSize);
			    } else {
				    return std::format(
				        "Bad construction. Invalid pointers provided. bufferPtr = {}; statePtr = {}",
				        static_cast<const void*>(bufferPtr),
				        static_cast<const void*>(statePtr));
			    }
		    });
	}
	/**
	 * @param buffer the provided storage memory buffer
	 * The size of `buffer` must be a power of 2. If not, the object
	 * created by it will be invalid
	 */
	RingBufferOverlay(std::span<T> buffer, StateStorage* statePtr)
	    : tx::RingBufferOverlay<T>(buffer.data(), buffer.size(), statePtr) {}
	/**
	 * Default Constructor that produces an object in null state
	 */
	RingBufferOverlay() : m_data(nullptr), m_state(nullptr), m_size(0) {}
	/**
     * Note on the destructor:
	 * It intentionally did not call the clear() function, because this class
	 * is just an overlay, it should not do operations that user did not ask
	 * for.
	 * But overall, the clear() operation is still necessary to be called
	 * before the overlay is destroied, unless specialized condition is applied
     */
	~RingBufferOverlay() {}

	RingBufferOverlay(const RingBufferOverlay&) = default;
	RingBufferOverlay& operator=(const RingBufferOverlay&) = default;
	RingBufferOverlay(RingBufferOverlay&& other) = default;
	RingBufferOverlay& operator=(RingBufferOverlay&& other) = default;

	bool valid() const { return m_data && m_size && m_state; }

	// basic getter

	bool full() const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		return size() == m_size;
	}
	bool empty() const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		return m_state->begin == m_state->end;
	}
	u32 size() const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		return m_state->end - m_state->begin;
	}
	u32 capacity() const { return m_size; }
	T* data() { return m_data; }
	const T* data() const { return m_data; }

	// single operations

	template <class U>
	    requires std::is_constructible_v<T, U&&>
	void push_back(U&& val) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_full(size(), m_size));
		std::construct_at(m_data + findPhysIndex_impl(m_state->end), std::forward<U>(val));
		m_state->end++;
	}
	template <class... Args>
	void emplace_back(Args&&... args) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_full(size(), m_size));
		std::construct_at(m_data + findPhysIndex_impl(m_state->end), std::forward<Args>(args)...);
		m_state->end++;
	}

	void pop_back() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_empty(size()));
		m_state->end--;
		std::destroy_at(m_data + findPhysIndex_impl(m_state->end));
	}

	template <class U>
	    requires std::is_constructible_v<T, U&&>
	void push_front(U&& val) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_full(size(), m_size));
		m_state->begin--;
		std::construct_at(m_data + findPhysIndex_impl(m_state->begin), std::forward<U>(val));
	}
	template <class... Args>
	void emplace_front(Args&&... args) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_full(size(), m_size));
		m_state->begin--;
		std::construct_at(m_data + findPhysIndex_impl(m_state->begin), std::forward<Args>(args)...);
	}

	void pop_front() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_empty(size()));
		std::destroy_at(m_data + findPhysIndex_impl(m_state->begin));
		m_state->begin++;
	}

	// single data getters

	template <class Self>
	decltype(auto) front(this Self&& self) {
		impl::assert_impl(impl::assert::overlay_object_valid(&self));
		impl::assert_impl(impl::assert::buffer_not_empty(self.size()));
		tx::const_propagate<Self, T>* ptr = self.m_data;
		return *(ptr + self.findPhysIndex_impl(self.m_state->begin));
	}
	template <class Self>
	decltype(auto) back(this Self&& self) {
		impl::assert_impl(impl::assert::overlay_object_valid(&self));
		impl::assert_impl(impl::assert::buffer_not_empty(self.size()));
		tx::const_propagate<Self, T>* ptr = self.m_data;
		return *(ptr + self.findPhysIndex_impl(self.m_state->end - 1));
	}

	// multi and random access operations

	template <class Self>
	decltype(auto) operator[](this Self&& self, u32 index) {
		impl::assert_impl(impl::assert::overlay_object_valid(&self));
		impl::assert_impl(impl::assert::out_of_range(self.size(), index));
		tx::const_propagate<Self, T>* ptr = self.m_data;
		return *(ptr + self.findPhysIndex_impl(self.m_state->begin + index));
	}

	void clear() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		if (empty()) return;
		u32 physBegin = findPhysIndex_impl(m_state->begin);
		u32 physEnd = findPhysIndex_impl(m_state->end);
		if (physBegin < physEnd) {
			// linear
			std::destroy(m_data + physBegin,
			             m_data + physEnd);
		} else if (physBegin > physEnd) {
			// wrap
			std::destroy(m_data,
			             m_data + physEnd);
			std::destroy(m_data + physBegin,
			             m_data + m_size);
		} else {
			// completely full
			// the empty edge case is prevented by the empty() check at the top
			std::destroy(m_data,
			             m_data + m_size);
		}
		m_state->begin = 0;
		m_state->end = 0;
	}

public:
	// lifetime APIs

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

	// ################ Relocation ################

	// includes data move
	// also flattens (unwraps) the buffer
	// @return if the buffer size is valid (isPowTwo), true if is pow2
	bool relocate(T* newBuffer, u32 newBufferSize) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::overlay_relocation_buffer_too_small(
		    newBufferSize, [this] { return this->size(); }));
		if (!tx::isPowTwo(newBufferSize)) return false;
		flattenRelocate_impl(newBuffer, m_data, m_size, m_state);
		m_state->end = size();
		m_state->begin = 0;
		m_data = newBuffer;
		m_size = newBufferSize;
		return true;
	}
	// excludes data move
	// @return if the buffer size is valid (isPowTwo), true if is pow2
	bool rebind(T* newBuffer, u32 newBufferSize) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::overlay_relocation_buffer_too_small(
		    newBufferSize, [this] { return this->size(); }));
		if (!tx::isPowTwo(newBufferSize)) return false;
		m_data = newBuffer;
		m_size = newBufferSize;
		return true;
	}

private:
	T* m_data;
	State_impl* m_state;
	u32 m_size;

private:
	// helpers

	// find physical index
	u32 findPhysIndex_impl(u32 index) const {
		return impl::findPowTwoWrappedPhysIndex(index, m_size);
	}

	static void flattenRelocate_impl(T* dest, T* data, u32 dataBufferSize, const State_impl* state) {
		if (state->begin == state->end) return;
		u32 physBegin = impl::findPowTwoWrappedPhysIndex(state->begin, dataBufferSize);
		u32 physEnd = impl::findPowTwoWrappedPhysIndex(state->end, dataBufferSize);
		if (physBegin < physEnd) {
			// linear
			tx::uninitialized_relocate(
			    data + physBegin,
			    data + physEnd,
			    dest);
		} else {
			// wrap
			tx::uninitialized_relocate(
			    data + physBegin,
			    data + dataBufferSize,
			    dest);
			tx::uninitialized_relocate(
			    data,
			    data + physEnd,
			    dest + (dataBufferSize - physBegin));
		}
	}

protected:
	// destroy the current overlay object
	// other copies of this overlay are not synced; it is not recommended to
	// call this function on overlay that is not unique
	void null_impl() {
		m_data = nullptr;
		m_state = nullptr;
		m_size = 0;
	}
	void swap_impl(RingBufferOverlay<T>& other) {
		std::swap(m_data, other.m_data);
		std::swap(m_size, other.m_size);
		std::swap(m_state->begin, other.m_state->begin);
		std::swap(m_state->end, other.m_state->end);
	}
	// copy the data and state of another object after construction
	// to fully sync with the other object
	void copy_impl(const RingBufferOverlay<T>& other) {
		m_state->begin = other.m_state->begin;
		m_state->end = other.m_state->end;
		if (other.empty()) return;
		u32 physBegin = other.findPhysIndex_impl(other.m_state->begin);
		u32 physEnd = other.findPhysIndex_impl(other.m_state->end);
		if (physBegin < physEnd) {
			// linear
			std::uninitialized_copy(
			    other.m_data + physBegin,
			    other.m_data + physEnd,
			    this->m_data + physBegin);
		} else if (physBegin > physEnd) {
			// wrap
			std::uninitialized_copy(
			    other.m_data,
			    other.m_data + physEnd,
			    this->m_data);
			std::uninitialized_copy(
			    other.m_data + physBegin,
			    other.m_data + m_size,
			    this->m_data + physBegin);
		} else {
			// completely full
			// the empty edge case is prevented by the empty() check at the top
			std::uninitialized_copy(
			    other.m_data,
			    other.m_data + m_size,
			    this->m_data);
		}
	}
	// query if object is valid
	// used for defend moved-from object
	bool isNull_impl() const {
		return !m_data;
	}
	// called at destruction to clean up data
	void destruct_impl() {
		clear();
	}
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