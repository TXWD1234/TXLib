// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/basic_utils.hpp"
#include "impl/data_utils.hpp"
#include <span>
#include <memory>

namespace tx {
/**
 * @brief A basic wrapper for heap memory
 * Any misuse of this class is the user's responsibility to handle the problem
 * For operations such as `construct()`, `destroy` and `operator[]`, if used
 * incorrectly could result UB, currputed data or resource leak.
 */
template <class T>
class BasicStorage {
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
	BasicStorage(u32 size)
	    : m_data(static_cast<T*>(::operator new(
	          size * sizeof(T), std::align_val_t{ alignof(T) }))),
	      m_size(size) {}
	~BasicStorage() {
		if (m_data)
			free_impl();
	}

	// regular APIs

	u32 size() const { return m_size; }

	T* data() { return m_data; }
	const T* data() const { return m_data; }

	T& operator[](u32 index) {
		assert_impl([&]() { return index < m_size; },
		            "tx::BasicStorage::operator[] subscript out of range.");
		return *(m_data + index);
	}
	const T& operator[](u32 index) const {
		assert_impl([&]() { return index < m_size; },
		            "tx::BasicStorage::operator[] subscript out of range.");
		return *(m_data + index);
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

	// utility APIs
	// the iterator ones exist only for conveniency

	// UB when called on initialized element / already constructed element
	template <class... Args>
	void construct(u32 index, Args&&... args) {
		assert_impl([&]() { return index < m_size; },
		            "tx::BasicStorage::construct() subscript out of range.");
		std::construct_at(m_data + index, std::forward<Args>(args)...);
	}
	// UB when called on initialized element / already constructed element
	template <class... Args>
	void construct(ConstIt_t it, Args&&... args) {
		u32 index = findIteratorIndex(this->cbegin(), it);
		construct(index, std::forward<Args>(args)...);
	}

	// UB when called on uninitialized element / already destroied element
	void destroy(u32 index) {
		assert_impl([&]() { return index < m_size; },
		            "tx::BasicStorage::destroy() subscript out of range.");
		std::destroy_at(m_data + index);
	}
	// UB when called on uninitialized element / already destroied element
	void destroy(ConstIt_t it) {
		u32 index = findIteratorIndex(this->cbegin(), it);
		destroy(index);
	}

	// UB when called on initialized element / already constructed element
	void construct(u32 indexBegin, u32 indexEnd) {
		assert_impl([&]() { return indexBegin < m_size && indexEnd <= m_size; },
		            "tx::BasicStorage::construct(): subscript out of range.");
		assert_impl([&]() { return indexBegin <= indexEnd; },
		            "tx::BasicStorage::construct(): Invalid range [begin, end). Begin index is greater than end index.");
		std::uninitialized_value_construct(m_data + indexBegin, m_data + indexEnd);
	}
	// UB when called on initialized element / already constructed element
	void construct(ConstIt_t begin, ConstIt_t end) {
		u32 indexBegin = findIteratorIndex(this->cbegin(), begin);
		u32 indexEnd = findIteratorIndex(this->cbegin(), end);
		construct(indexBegin, indexEnd);
	}
	// UB when called on initialized element / already constructed element
	void construct_no_zero_init(u32 indexBegin, u32 indexEnd) {
		assert_impl([&]() { return indexBegin < m_size && indexEnd <= m_size; },
		            "tx::BasicStorage::construct(): subscript out of range.");
		assert_impl([&]() { return indexBegin <= indexEnd; },
		            "tx::BasicStorage::construct(): Invalid range [begin, end). Begin index is greater than end index.");
		std::uninitialized_default_construct(m_data + indexBegin, m_data + indexEnd);
	}
	// UB when called on initialized element / already constructed element
	void construct_no_zero_init(ConstIt_t begin, ConstIt_t end) {
		u32 indexBegin = findIteratorIndex(this->cbegin(), begin);
		u32 indexEnd = findIteratorIndex(this->cbegin(), end);
		construct_no_zero_init(indexBegin, indexEnd);
	}

	// UB when called on uninitialized element / already destroied element
	void destroy(u32 indexBegin, u32 indexEnd) {
		assert_impl([&]() { return indexBegin < m_size && indexEnd <= m_size; },
		            "tx::BasicStorage::destroy(): subscript out of range.");
		assert_impl([&]() { return indexBegin <= indexEnd; },
		            "tx::BasicStorage::destroy(): Invalid range [begin, end). Begin index is greater than end index.");
		std::destroy(m_data + indexBegin, m_data + indexEnd);
	}
	// UB when called on uninitialized element / already destroied element
	void destroy(ConstIt_t begin, ConstIt_t end) {
		u32 indexBegin = findIteratorIndex(this->cbegin(), begin);
		u32 indexEnd = findIteratorIndex(this->cbegin(), end);
		destroy(indexBegin, indexEnd);
	}

private:
	T* m_data;
	u32 m_size;

	void free_impl() {
		::operator delete(m_data, std::align_val_t{ alignof(T) });
	}
};
} // namespace tx