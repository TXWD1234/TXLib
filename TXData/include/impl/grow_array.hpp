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
// std::vector but fixed capacity
// std:span-like overlay that operates a piece of memory user assigned.
template <class T>
class GrowArrayOverlay {
private:
	// using std::span's iterator for debug build bound check assertion
	using It_t = typename std::span<T>::iterator;
	/**
	 * Intentianl use of std::span<T>::const_iterator instead of
	 * std::span<const T>::iterator to keep iterator the same type, therefore
	 * convertible between each other
	 */
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
	GrowArrayOverlay(T* ptr, u32 capacity)
	    : m_data(ptr), m_size(0),
	      m_capacity(capacity) {}
	/**
	 * @param buffer the provided storage memory buffer
	 * Note that this constructorcan only result a non-owning object
	 */
	GrowArrayOverlay(std::span<T> buffer)
	    : m_data(buffer.data()), m_size(0),
	      m_capacity(buffer.size()) {}
	~GrowArrayOverlay() {}

	GrowArrayOverlay(const GrowArrayOverlay&) = default;
	GrowArrayOverlay& operator=(const GrowArrayOverlay&) = default;
	GrowArrayOverlay(GrowArrayOverlay&& other) = default;
	GrowArrayOverlay& operator=(GrowArrayOverlay&& other) = default;

	u32 size() const { return m_size; }
	u32 capacity() const { return m_capacity; }
	bool empty() const { return m_size == 0; }

	T* data() { return m_data; }
	const T* data() const { return m_data; }

	void resize(u32 newSize) {
		assert_impl([&]() { return newSize <= m_capacity; },
		            "tx::GrowArrayOverlay::resize(): requested newSize overflows capacity.");
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
		assert_impl([&]() { return newSize <= m_capacity; },
		            "tx::GrowArrayOverlay::resize_no_zero_init(): requested newSize overflows capacity.");
		if (newSize < m_size) {
			std::destroy(m_data + newSize, m_data + m_size);
		} else {
			std::uninitialized_default_construct(m_data + m_size, m_data + newSize);
		}
		m_size = newSize;
	}

	It_t erase(ConstIt_t it) {
		u32 index = findIteratorIndex(this->cbegin(), it);
		assert_impl([&]() { return index < m_size; },
		            "tx::GrowArrayOverlay::::erase(): subscript out of range.");
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
		assert_impl([&]() { return indexBegin < m_size && indexEnd <= m_size; },
		            "tx::GrowArrayOverlay::erase(): subscript out of range.");
		assert_impl([&]() { return indexBegin <= indexEnd; },
		            "tx::GrowArrayOverlay::erase(): Invalid range [begin, end). Begin index is greater than end index.");
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
		assert_impl([&]() { return m_size < m_capacity; },
		            "tx::GrowArrayOverlay::push_back(): called on full StaticGrowArr.");
		std::construct_at(m_data + m_size, val);
		m_size++;
	}
	void push_back(T&& val) {
		assert_impl([&]() { return m_size < m_capacity; },
		            "tx::GrowArrayOverlay::push_back(): called on full StaticGrowArr.");
		std::construct_at(m_data + m_size, std::move(val));
		m_size++;
	}
	template <class... Args>
	void emplace_back(Args&&... args) {
		assert_impl([&]() { return m_size < m_capacity; },
		            "tx::GrowArrayOverlay::emplace_back(): called on full StaticGrowArr.");
		std::construct_at(m_data + m_size, std::forward<Args>(args)...);
		m_size++;
	}
	void pop_back() {
		assert_impl([&]() { return m_size > 0; },
		            "tx::GrowArrayOverlay::pop_back(): called on empty StaticGrowArr.");
		m_size--;
		std::destroy_at(m_data + m_size);
	}

	T& operator[](u32 index) {
		assert_impl([&]() { return index < m_size; },
		            "tx::GrowArrayOverlay::operator[]: subscript out of range.");
		return *(m_data + index);
	}
	const T& operator[](u32 index) const {
		assert_impl([&]() { return index < m_size; },
		            "tx::GrowArrayOverlay::operator[]: subscript out of range.");
		return *(m_data + index);
	}

	T& front() {
		assert_impl([&]() { return m_size > 0; },
		            "tx::GrowArrayOverlay::front(): called on empty StaticGrowArr.");
		return *m_data;
	}
	const T& front() const {
		assert_impl([&]() { return m_size > 0; },
		            "tx::GrowArrayOverlay::front(): called on empty StaticGrowArr.");
		return *m_data;
	}
	T& back() {
		assert_impl([&]() { return m_size > 0; },
		            "tx::GrowArrayOverlay::back(): called on empty StaticGrowArr.");
		return *(m_data + m_size - 1);
	}
	const T& back() const {
		assert_impl([&]() { return m_size > 0; },
		            "tx::GrowArrayOverlay::back(): called on empty StaticGrowArr.");
		return *(m_data + m_size - 1);
	}


	std::span<T> span() { return std::span<T>(m_data, m_size); }
	std::span<const T> span() const { return std::span<const T>(m_data, m_size); }

	/**
	 * Have to inline span() here since const function cannot call non-const span
	 */

	It_t begin() { return span().begin(); }
	ConstIt_t begin() const { return std::span<T>(m_data, m_size).cbegin(); }
	It_t end() { return span().end(); }
	ConstIt_t end() const { return std::span<T>(m_data, m_size).cend(); }
	ConstIt_t cbegin() const { return std::span<T>(m_data, m_size).cbegin(); }
	ConstIt_t cend() const { return std::span<T>(m_data, m_size).cend(); }

protected:
	T* m_data;
	u32 m_size,
	    m_capacity;
};

template <class T>
class GrowArray : public GrowArrayOverlay<T> {
public:
	GrowArray(u32 capacity)
	    : GrowArrayOverlay<T>(
	          static_cast<T*>(::operator new(
	              capacity * sizeof(T), std::align_val_t{ alignof(T) })),
	          capacity) {}
	~GrowArray() {
		free_impl();
	}

	GrowArray(const GrowArray<T>& other)
	    : GrowArrayOverlay<T>(
	          static_cast<T*>(::operator new(
	              other.m_capacity * sizeof(T), std::align_val_t{ alignof(T) })),
	          other.m_capacity) {
		this->m_size = other.m_size;
		std::uninitialized_copy(other.begin(), other.end(), this->begin());
	}
	GrowArray& operator=(const GrowArray<T>& other) {
		if (this == &other) return *this;
		free_impl();
		this->m_data = static_cast<T*>(::operator new(
		    other.m_capacity * sizeof(T), std::align_val_t{ alignof(T) }));
		this->m_capacity = other.m_capacity;
		this->m_size = other.m_size;
		std::uninitialized_copy(other.begin(), other.end(), this->begin());
		return *this;
	}
	GrowArray(GrowArray<T>&& other) : GrowArrayOverlay<T>(other) {
		// just use the copy constructor of GrowArrayOverlay - shallow copy
		other.m_data = nullptr;
		other.m_capacity = 0;
		other.m_size = 0;
	}
	GrowArray& operator=(GrowArray<T>&& other) {
		if (this == &other) return *this;
		free_impl();
		this->m_data = other.m_data;
		this->m_capacity = other.m_capacity;
		this->m_size = other.m_size;
		other.m_data = nullptr;
		other.m_capacity = 0;
		other.m_size = 0;
		return *this;
	}

private:
	void free_impl() {
		this->clear(); // destroy live elements before freeing
		::operator delete(this->data(), std::align_val_t{ alignof(T) });
	}
};

template <class T>
using static_vector = GrowArray<T>;



} // namespace tx