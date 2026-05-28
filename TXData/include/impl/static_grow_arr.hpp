// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/basic_utils.hpp"
#include "impl/data_utils.hpp"
#include "tx/type_traits.hpp"
#include <span>
#include <stdexcept>
#include <memory>
#include <type_traits>

namespace tx {

// this is scheduled to move to TXFoundation immediately after TXLib structural refactor
#ifdef NDEBUG
inline constexpr const bool enabled_debug = false;
#else
inline constexpr const bool enabled_debug = true;
#endif

#ifdef __cpp_exceptions
inline constexpr const bool enabled_exception = true;
#else
inline constexpr const bool enabled_exception = false;
#endif

template <tx::invocable_r<bool> Expr>
inline static void assert_impl(Expr&& expr, const char* message) {
	if constexpr (enabled_debug && enabled_exception) {
		if (!expr()) [[unlikely]]
			throw std::runtime_error(message);
	}
}



// std::vector but fixed capacity
// copy is disabled, this class is move-only
template <class T>
class StaticGrowArr {
private:
	// using std::span's iterator for debug build bound check assertion
	using It_t = typename std::span<T>::iterator;
	using ConstIt_t = typename std::span<T>::const_iterator;

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
	/**
	 * @param buffer the provided storage memory buffer
	 * Note that this constructorcan only result a non-owning object
	 */
	StaticGrowArr(std::span<T> buffer)
	    : m_data(buffer.data()),
	      m_size(0), m_capacity(buffer.size()), m_ownsMemory(false) {}
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

	T* data() { return m_data; }
	const T* data() const { return m_data; }

	void resize(u32 newSize) {
		assert_impl([&]() { return newSize <= m_capacity; }, "requested newSize overflows capacity.");
		if (newSize < m_size) {
			std::destroy(m_data + newSize, m_data + m_size);
		} else {
			std::uninitialized_value_construct(m_data + m_size, m_data + newSize);
		}
		m_size = newSize;
	}
	// resize but don't zero init PODs
	// exist only for performance
	void resize_no_zero_init(u32 newSize) {
		assert_impl([&]() { return newSize <= m_capacity; }, "requested newSize overflows capacity.");
		if (newSize < m_size) {
			std::destroy(m_data + newSize, m_data + m_size);
		} else {
			std::uninitialized_default_construct(m_data + m_size, m_data + newSize);
		}
		m_size = newSize;
	}

	It_t erase(ConstIt_t it) {
		u32 index = findIteratorIndex(this->cbegin(), it);
		assert_impl([&]() { return index < m_size; }, "StaticGrowArr::erase() subscript out of range.");
		It_t mit = this->begin() + index; // mutable it
		m_size--;
		if (index != m_size)
			std::move(mit + 1, this->end(), mit);
		std::destroy_at(m_data + m_size);
		return mit;
	}
	It_t erase(ConstIt_t begin, ConstIt_t end) {
		u32 indexBegin = findIteratorIndex(this->cbegin(), begin);
		u32 indexEnd = findIteratorIndex(this->cbegin(), end);
		assert_impl([&]() { return indexBegin < m_size && indexEnd <= m_size; }, "StaticGrowArr::erase() subscript out of range.");
		assert_impl([&]() { return indexBegin < indexEnd; }, "StaticGrowArr::erase() begin is greater then end.");
		It_t mbegin = this->begin() + indexBegin; // mutable begin
		It_t mend = this->begin() + indexEnd; // mutable end
		u32 eraseSize = static_cast<u32>(std::distance(begin, end));
		if (indexEnd == m_size)
			std::destroy(mbegin, mend);
		else {
			std::move(mend, this->end(), mbegin);
			std::destroy(m_data + m_size - eraseSize, m_data + m_size);
		}
		m_size -= eraseSize;
		return mbegin;
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


	std::span<T> span() { return std::span<T>(m_data, m_size); }
	std::span<const T> span() const { return std::span<const T>(m_data, m_size); }

	It_t begin() { return span().begin(); }
	ConstIt_t begin() const { return std::span<T>(m_data, m_size).cbegin(); }
	It_t end() { return span().end(); }
	ConstIt_t end() const { return std::span<T>(m_data, m_size).cend(); }
	ConstIt_t cbegin() const { return std::span<T>(m_data, m_size).cbegin(); }
	ConstIt_t cend() const { return std::span<T>(m_data, m_size).cend(); }

	// have to inline the `span()` function for `ConstIt_t begin() const` and
	// `ConstIt_t begin() const` because they cannot call normal `span()`,
	// meanwhile `span() const` should return `std::span<const T>`, which is a
	// different type then `std::span<T>`

private:
	T* m_data;
	u32 m_size,
	    m_capacity;
	bool m_ownsMemory;

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