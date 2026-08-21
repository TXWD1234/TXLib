// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "tx/basic_types.hpp"
#include "impl/data_utils.hpp" // include for the exception
#include "impl/numeric_utils.hpp"
#include <span>
#include <memory>

namespace tx {
/**
 * Everytime before you push, you should check `full()`.
 *   Any push operation on an full buffer is Undefined Behavior
 * Everytime before you pop, you should check `empty()`.
 *   Any pop operation on an empty buffer is Undefined Behavior
 */
template <class T>
class RingBufferOverlay {
public:
	using value_type = T;

public:
	/**
	 * @param ptr the data pointer to a piece of memory that have at least
	 * size of `capacity`.
	 * @param size the capacity of this container object. It cannot resize.
	 * It have to be a power of 2. If not, the object created by it will be
	 * invalid
	 * DevNote: maybe fallback to the maximum power of 2 that's smaller then
	 * size?
	 */
	RingBufferOverlay(T* ptr, u32 size)
	    : m_data(tx::isPowTwo(size) ? ptr : nullptr),
	      m_size(tx::isPowTwo(size) ? size : 0) {
		impl::assert_impl([&]() { return valid(); },
		                  "tx::RingBufferOverlay::RingBufferOverlay: invalid object");
	}
	/**
	 * @param buffer the provided storage memory buffer
	 * The size of `buffer` have to be a power of 2. If not, the object
	 * created by it will be invalid
	 * DevNote: maybe fallback to the maximum power of 2 that's smaller then
	 * size?
	 */
	RingBufferOverlay(std::span<T> buffer)
	    : m_data(tx::isPowTwo(buffer.size()) ? buffer.data() : nullptr),
	      m_size(tx::isPowTwo(buffer.size()) ? buffer.size() : 0) {
		impl::assert_impl([&]() { return valid(); },
		                  "tx::RingBufferOverlay::RingBufferOverlay: invalid object");
	}
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

	bool valid() const { return m_data != nullptr && m_size != 0; }

	// basic getter

	bool full() const { return size() == m_size; }
	bool empty() const { return m_begin == m_end; }
	u32 size() const { return m_end - m_begin; }
	u32 capacity() const { return m_size; }

	T* data() { return m_data; }
	const T* data() const { return m_data; }

	// single operations

	template <class U>
	    requires std::is_constructible_v<T, U&&>
	void push_back(U&& val) {
		impl::assert_impl([&]() { return !full(); },
		                  "tx::RingBufferOverlay::push_back(): called on full buffer");
		std::construct_at(m_data + findPhysIndex(m_end), std::forward<U>(val));
		m_end++;
	}
	template <class... Args>
	void emplace_back(Args&&... args) {
		impl::assert_impl([&]() { return !full(); },
		                  "tx::RingBufferOverlay::emplace_back(): called on full buffer");
		std::construct_at(m_data + findPhysIndex(m_end), std::forward<Args>(args)...);
		m_end++;
	}

	void pop_back() {
		impl::assert_impl([&]() { return !empty(); },
		                  "tx::RingBufferOverlay::pop_back(): called on empty buffer");
		m_end--;
		std::destroy_at(m_data + findPhysIndex(m_end));
	}

	template <class U>
	    requires std::is_constructible_v<T, U&&>
	void push_front(U&& val) {
		impl::assert_impl([&]() { return !full(); },
		                  "tx::RingBufferOverlay::push_front(): called on full buffer");
		m_begin--;
		std::construct_at(m_data + findPhysIndex(m_begin), std::forward<U>(val));
	}
	template <class... Args>
	void emplace_front(Args&&... args) {
		impl::assert_impl([&]() { return !full(); },
		                  "tx::RingBufferOverlay::emplace_front(): called on full buffer");
		m_begin--;
		std::construct_at(m_data + findPhysIndex(m_begin), std::forward<Args>(args)...);
	}

	void pop_front() {
		impl::assert_impl([&]() { return !empty(); },
		                  "tx::RingBufferOverlay::pop_front(): called on empty buffer");
		std::destroy_at(m_data + findPhysIndex(m_begin));
		m_begin++;
	}

	// single data getters

	template <class Self>
	decltype(auto) front(this Self&& self) {
		impl::assert_impl([&]() { return !self.empty(); },
		                  "tx::RingBufferOverlay::front(): called on empty buffer");
		return *(self.m_data + self.findPhysIndex(self.m_begin));
	}
	template <class Self>
	decltype(auto) back(this Self&& self) {
		impl::assert_impl([&]() { return !self.empty(); },
		                  "tx::RingBufferOverlay::back(): called on empty buffer");
		return *(self.m_data + self.findPhysIndex(self.m_end - 1));
	}

	// multi and random access operations

	template <class Self>
	decltype(auto) operator[](this Self&& self, u32 index) {
		impl::assert_impl([&]() { return index < self.size(); },
		                  "tx::RingBufferOverlay::operator[]: subscript out of range");
		return *(self.m_data + self.findPhysIndex(self.m_begin + index));
	}

	void clear() {
		if (empty()) return;
		u32 physBegin = findPhysIndex(m_begin);
		u32 physEnd = findPhysIndex(m_end);
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
		m_begin = 0;
		m_end = 0;
	}

private:
	T* m_data;
	u32 m_size;
	u32 m_begin = 0, m_end = 0;

private:
	// helpers

	// find physical index
	u32 findPhysIndex(u32 index) const {
		return index & (m_size - 1);
	}

protected:
	void null_impl() {
		m_data = nullptr;
		m_size = 0;
		m_begin = 0;
		m_end = 0;
	}
	void swap_impl(RingBufferOverlay<T>& other) {
		std::swap(m_data, other.m_data);
		std::swap(m_size, other.m_size);
		std::swap(m_begin, other.m_begin);
		std::swap(m_end, other.m_end);
	}
	// copy the data and state of another object after construction
	// to fully sync with the other object
	void copy_impl(const RingBufferOverlay<T>& other) {
		m_begin = other.m_begin;
		m_end = other.m_end;
		if (other.empty()) return;
		u32 physBegin = other.findPhysIndex(other.m_begin);
		u32 physEnd = other.findPhysIndex(other.m_end);
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

template <class T, u32 size>
    requires(size > 0 && (size & (size - 1)) == 0)
class RingBuffer : public RingBufferOverlay<T> {
public:
	RingBuffer()
	    : RingBufferOverlay<T>(
	          allocate<T>(size), size) {}
	~RingBuffer() {
		if (!this->isNull_impl()) {
			this->destruct_impl(); // destroy live elements before freeing
			free(this->data());
		}
	}

	RingBuffer(const RingBuffer<T, size>& other)
	    : RingBufferOverlay<T>(
	          allocate<T>(other.capacity()), other.capacity()) {
		copy_impl(other);
	}
	RingBuffer(RingBuffer<T, size>&& other) : RingBufferOverlay<T>(other) {
		// just use the copy constructor of RingBufferOverlay - shallow copy
		other.null_impl();
	}
	RingBuffer& operator=(RingBuffer<T, size> other) {
		this->swap_impl(other);
		return *this;
	}

private:
	// cannot be swap_impl because ambiguity with base's swap_impl
	// this function exists for potential future expansion
	void swap(RingBuffer<T, size>& other) {
		swap_impl(other);
	}
};
} // namespace tx