// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "tx/basic_types.hpp"
#include "impl/data_utils.hpp" // include for the exception
#include <span>
#include <concepts>
#include <memory>

namespace tx {
/**
 * Everytime before you push, you should check `full()`.
 *   Any push operation on an full CircularQueue is Undefined Behavior
 * Everytime before you pop, you should check `empty()`.
 *   Any pop operation on an empty CircularQueue is Undefined Behavior
 */
template <class T>
class CircularQueueOverlay {
public:
	using value_type = T;

public:
	/**
	 * @param ptr the data pointer to a piece of memory that have at least size of `capacity`.
	 * @param capacity the capacity of this container object. It cannot resize.
	 */
	CircularQueueOverlay(T* ptr, u32 size)
	    : m_data(ptr), m_size(size) {}
	/**
	 * @param buffer the provided storage memory buffer
	 */
	CircularQueueOverlay(std::span<T> buffer)
	    : m_data(buffer.data()), m_size(buffer.size()) {}
	~CircularQueueOverlay() {}

	CircularQueueOverlay(const CircularQueueOverlay&) = default;
	CircularQueueOverlay& operator=(const CircularQueueOverlay&) = default;
	CircularQueueOverlay(CircularQueueOverlay&& other) = default;
	CircularQueueOverlay& operator=(CircularQueueOverlay&& other) = default;

	// basic operations

	void push(const T& val) {
		impl::assert_impl([&]() { return !full(); },
		                  "tx::CircularQueueOverlay::push(): called on full buffer");
		std::construct_at(m_data + m_end, val);
		push_impl();
	}
	void push(T&& val) {
		impl::assert_impl([&]() { return !full(); },
		                  "tx::CircularQueueOverlay::push(): called on full buffer");
		std::construct_at(m_data + m_end, std::move(val));
		push_impl();
	}
	template <class... Args>
	void emplace(Args&&... args) {
		impl::assert_impl([&]() { return !full(); },
		                  "tx::CircularQueueOverlay::emplace(): called on full buffer");
		std::construct_at(m_data + m_end, std::forward<Args>(args)...);
		push_impl();
	}

	void pop() {
		std::destroy_at(m_data + m_begin);
		pop_impl();
	}

	void clear() {
		if (m_wrap) {
			std::destroy(
			    m_data + m_begin,
			    m_data + m_size);
			std::destroy(
			    m_data,
			    m_data + m_end);
		} else {
			std::destroy(
			    m_data + m_begin,
			    m_data + m_end);
		}

		m_begin = 0;
		m_end = 0;
		m_wrap = false;
	}

	// basic getter

	bool full() const { return m_begin == m_end && m_wrap; }
	bool empty() const { return m_begin == m_end && !m_wrap; }
	u32 size() const {
		return (m_wrap ?
		            m_size - m_begin + m_end :
		            m_end - m_begin);
	}
	u32 capacity() const { return m_size; }

	T* data() { return m_data; }
	const T* data() const { return m_data; }

	// data getters

	T& front() {
		impl::assert_impl([&]() { return !empty(); },
		                  "tx::CircularQueueOverlay::front(): called on empty buffer");
		return *(m_data + m_begin);
	}
	const T& front() const {
		impl::assert_impl([&]() { return !empty(); },
		                  "tx::CircularQueueOverlay::front(): called on empty buffer");
		return *(m_data + m_begin);
	}
	T& back() {
		impl::assert_impl([&]() { return !empty(); },
		                  "tx::CircularQueueOverlay::back(): called on empty buffer");
		return *(m_data + m_end - 1);
	}
	const T& back() const {
		impl::assert_impl([&]() { return !empty(); },
		                  "tx::CircularQueueOverlay::back(): called on empty buffer");
		return *(m_data + m_end - 1);
	}

private:
	T* m_data;
	u32 m_size;
	u32 m_begin = 0, m_end = 0;
	bool m_wrap = false;

	T& at_impl(u32 index) {
		return *(m_data + index);
	}

	void push_impl() {
		m_end++;
		if (m_end == m_size) {
			m_end = 0; // wrapping logic
			m_wrap = true;
		}
	}
	void pop_impl() {
		m_begin++;
		if (m_begin == m_size) {
			m_begin = 0; // wrapping logic
			m_wrap = false;
		}
	}

	template <std::invocable<T&> Func>
	void foreach_impl(Func&& f) {
		if (m_wrap) {
			for (u32 i = m_begin; i < m_size; i++) {
				f(at_impl(i));
			}
			for (u32 i = 0; i < m_end; i++) {
				f(at_impl(i));
			}
		} else {
			for (u32 i = m_begin; i < m_end; i++) {
				f(at_impl(i));
			}
		}
	}

protected:
	void null_impl() {
		m_data = nullptr;
		m_size = 0;
		m_begin = 0;
		m_end = 0;
		m_wrap = false;
	}
	void swap_impl(CircularQueueOverlay<T>& other) {
		std::swap(m_data, other.m_data);
		std::swap(m_size, other.m_size);
		std::swap(m_begin, other.m_begin);
		std::swap(m_end, other.m_end);
		std::swap(m_wrap, other.m_wrap);
	}
	// copy the data and state of another object after construction
	// to fully sync with the other object
	void copy_impl(const CircularQueueOverlay<T>& other) {
		if (other.m_wrap) {
			std::uninitialized_copy(
			    other.m_data + other.m_begin,
			    other.m_data + other.m_size,
			    m_data + other.m_begin);
			std::uninitialized_copy(
			    other.m_data,
			    other.m_data + other.m_end,
			    m_data);
		} else {
			std::uninitialized_copy(
			    other.m_data + other.m_begin,
			    other.m_data + other.m_end,
			    m_data);
		}
		m_begin = other.m_begin;
		m_end = other.m_end;
		m_wrap = other.m_wrap;
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

template <class T>
class CircularQueue : public CircularQueueOverlay<T> {
public:
	CircularQueue(u32 capacity)
	    : CircularQueueOverlay<T>(
	          allocate<T>(capacity), capacity) {}
	~CircularQueue() {
		if (!this->isNull_impl()) {
			this->destruct_impl(); // destroy live elements before freeing
			free(this->data());
		}
	}

	CircularQueue(const CircularQueue<T>& other)
	    : CircularQueueOverlay<T>(
	          allocate<T>(other.capacity()), other.capacity()) {
		copy_impl(other);
	}
	CircularQueue(CircularQueue<T>&& other) : CircularQueueOverlay<T>(other) {
		// just use the copy constructor of CircularQueueOverlay - shallow copy
		other.null_impl();
	}
	CircularQueue& operator=(CircularQueue<T> other) {
		this->swap_impl(other);
		return *this;
	}

private:
	// cannot be swap_impl because ambiguity with base's swap_impl
	// this function exists for potential future expansion
	void swap(CircularQueue<T>& other) {
		swap_impl(other);
	}
};
} // namespace tx