// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/basic_utils.hpp"
#include "tx/type_traits.hpp"
#include <span>
#include <stdexcept>
#include <memory>

namespace tx {

// this is scheduled to move to TXFoundation immediately after TXLib structural refactor
#ifdef NDEBUG
inline constexpr const bool debug = false;
#else
inline constexpr const bool debug = true;
#endif

// std::vector but fixed capacity
// copy is disabled, this class is move-only
template <class T>
class StaticGrowArr {
private:
	using It_t = std::conditional_t<debug, typename std::span<T>::iterator, T*>;
	using ConstIt_t = std::conditional_t<debug, typename std::span<T>::const_iterator, const T*>;

public:
	using iterator = It_t;
	using const_iterator = ConstIt_t;

public:
	/**
	 * @param capacity the capacity of this container object. It cannot resize.
	 * @param ptr the data pointer to a piece of memory that have at least size of `capacity`.
	 * set to nullptr to let the object manage the memory
	 */
	StaticGrowArr(u32 capacity, T* ptr = nullptr)
	    : m_data(ptr ? ptr :
	                   static_cast<T*>(::operator new(
	                       capacity * sizeof(T), std::align_val_t{ alignof(T) }))),
	      m_size(0), m_capacity(capacity), m_ownsMemory(ptr == nullptr) {}
	StaticGrowArr(ConstIt_t begin, ConstIt_t end)
	    : m_data(std::to_address(begin)),
	      m_size(0), m_capacity(std::distance(begin, end)), m_ownsMemory(false) {}
	~StaticGrowArr() {
		this->free_impl();
	}

	// disable copy and move semantics
	StaticGrowArr(const StaticGrowArr&) = delete;
	StaticGrowArr& operator=(const StaticGrowArr&) = delete;
	StaticGrowArr(StaticGrowArr&& other)
	    : m_data(other.m_data),
	      m_size(other.m_size),
	      m_capacity(other.m_capacity),
	      m_ownsMemory(other.m_ownsMemory) { other.null_impl(); }
	StaticGrowArr& operator=(StaticGrowArr&& other) {
		if (this == &other) return *this;
		this->free_impl();
		m_data = other.m_data;
		m_size = other.m_size;
		m_capacity = other.m_capacity;
		m_ownsMemory = other.m_ownsMemory;
		other.null_impl();
		return *this;
	}


	u32 size() const { return m_size; }
	u32 capacity() const { return m_capacity; }
	bool empty() const { return m_size == 0; }

	void resize(u32 newSize) {
		assert_impl([&]() { return newSize <= m_capacity; }, "requested newSize overflows capacity.");
		if (newSize < m_size) {
			std::destroy(m_data + newSize, m_data + m_size);
		} else {
			std::uninitialized_value_construct(m_data + m_size, m_data + newSize);
		}
		m_size = newSize;
	}

	void clear() {
		std::destroy(m_data, m_data + m_size);
		m_size = 0;
	}

	void push_back(const T& val) {
		assert_impl([&]() { return m_size < m_capacity; }, "push_back() called on full StaticGrowArr.");
		std::construct_at(m_data + m_size, val);
		m_size++;
	}
	void push_back(T&& val) {
		assert_impl([&]() { return m_size < m_capacity; }, "push_back() called on full StaticGrowArr.");
		std::construct_at(m_data + m_size, std::move(val));
		m_size++;
	}
	template <class... Args>
	void emplace_back(Args&&... args) {
		assert_impl([&]() { return m_size < m_capacity; }, "emplace_back() called on full StaticGrowArr.");
		std::construct_at(m_data + m_size, std::forward<Args>(args)...);
		m_size++;
	}
	void pop_back() {
		assert_impl([&]() { return m_size > 0; }, "pop_back() called on empty StaticGrowArr.");
		m_size--;
		std::destroy_at(m_data + m_size);
	}

	T& operator[](u32 index) {
		assert_impl([&]() { return index < m_size; }, "StaticGrowArr::operator[] subscript out of range.");
		return *(m_data + index);
	}
	const T& operator[](u32 index) const {
		assert_impl([&]() { return index < m_size; }, "StaticGrowArr::operator[] subscript out of range.");
		return *(m_data + index);
	}

	T& front() {
		assert_impl([&]() { return m_size > 0; }, "front() called on empty StaticGrowArr.");
		return *m_data;
	}
	const T& front() const {
		assert_impl([&]() { return m_size > 0; }, "front() called on empty StaticGrowArr.");
		return *m_data;
	}
	T& back() {
		assert_impl([&]() { return m_size > 0; }, "back() called on empty StaticGrowArr.");
		return *(m_data + m_size - 1);
	}
	const T& back() const {
		assert_impl([&]() { return m_size > 0; }, "back() called on empty StaticGrowArr.");
		return *(m_data + m_size - 1);
	}

	It_t begin() { return It_t{ m_data }; }
	ConstIt_t begin() const { return ConstIt_t{ m_data }; }
	It_t end() { return It_t{ m_data + m_size }; }
	ConstIt_t end() const { return ConstIt_t{ m_data + m_size }; }

private:
	T* m_data;
	u32 m_size,
	    m_capacity;
	bool m_ownsMemory;

	template <tx::invocable_r<bool> Expr>
	inline static void assert_impl(Expr&& expr, const char* message) {
		if constexpr (debug) {
			if (!expr()) [[unlikely]]
				throw std::runtime_error(message);
		}
	}

	void null_impl() {
		m_data = nullptr;
		m_size = 0;
		m_capacity = 0;
		m_ownsMemory = false;
	}
	void free_impl() {
		if (m_ownsMemory) {
			this->clear();
			::operator delete(m_data, std::align_val_t{ alignof(T) });
		}
	}
};
template <class T>
using static_vector = StaticGrowArr<T>;


} // namespace tx