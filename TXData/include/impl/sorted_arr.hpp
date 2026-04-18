// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXData
// File: sorted_arr.h

#pragma once
#include "impl/basic_utils.hpp"
#include "tx/type_traits.hpp"
#include "tx/algorithm.hpp"
#include <algorithm>
#include <initializer_list>
#include <span>
#include <vector>

namespace tx {

template <class T, tx::invocable_r<bool, T, T> CmpFunc = std::less<T>>
class SortedArr {
	class Sort_impl;

public:
	using value_type = T;

	using It_t = typename std::vector<T>::iterator;
	using ConstIt_t = typename std::vector<T>::const_iterator;

	SortedArr(CmpFunc&& cmp = CmpFunc{})
	    : cmp(std::move(cmp)) {}
	SortedArr(size_t size, CmpFunc&& cmp = CmpFunc{})
	    : m_data(size), cmp(std::move(cmp)) { sort_impl(); }
	SortedArr(size_t size, const T& value, CmpFunc&& cmp = CmpFunc{})
	    : m_data(size, value), cmp(std::move(cmp)) { sort_impl(); }
	template <tx::input_iterator_value_type<T> It>
	SortedArr(It begin, It end, CmpFunc&& cmp = CmpFunc{})
	    : m_data(begin, end), cmp(std::move(cmp)) {
		sort_impl();
	}
	SortedArr(std::span<const T> data, CmpFunc&& cmp = CmpFunc{})
	    : m_data(data.begin(), data.end()), cmp(std::move(cmp)) {
		sort_impl();
	}
	SortedArr(std::initializer_list<T> ilist, CmpFunc&& cmp = CmpFunc{})
	    : m_data(ilist), cmp(std::move(cmp)) {
		sort_impl();
	}

	SortedArr(const SortedArr&) = default;
	SortedArr(SortedArr&&) noexcept = default;
	SortedArr& operator=(const SortedArr&) = default;
	SortedArr& operator=(SortedArr&&) noexcept = default;

	const T& operator[](size_t index) const { return m_data[index]; }

	/**
	 * Note:
	 * no direct non-const access allowed to protect the constaint: array must be sorted
	 */

	size_t size() const { return m_data.size(); }
	bool empty() const { return m_data.empty(); }
	size_t capacity() const { return m_data.capacity(); }
	const T* data() const { return m_data.data(); }
	void clear() { m_data.clear(); }
	void reserve(size_t size) { m_data.reserve(size); }

	ConstIt_t begin() const { return m_data.begin(); }
	ConstIt_t end() const { return m_data.end(); }

	const T& front() const { return m_data.front(); }
	const T& back() const { return m_data.back(); }

	ConstIt_t find(const T& value) const {
		ConstIt_t it = find_impl(value);
		if (validIt_impl(it, value)) return it;
		return m_data.end();
	}
	ConstIt_t find(u32 begin, const T& value) const {
		ConstIt_t it = find_impl(begin, m_data.size(), value);
		if (validIt_impl(it, value)) return it;
		return m_data.end();
	}
	ConstIt_t find(u32 begin, u32 count, const T& value) const {
		ConstIt_t it = find_impl(begin, begin + count, value);
		if (validIt_impl(it, value)) return it;
		return m_data.end();
	}
	ConstIt_t find(ConstIt_t first, ConstIt_t last, const T& value) const {
		ConstIt_t it = std::lower_bound(first, last, value, cmp);
		if (it != last && isSame_impl(*it, value)) return it;
		return last;
	}

	ConstIt_t lower_bound(const T& value) const {
		return find_impl(value);
	}
	ConstIt_t lower_bound(u32 begin, const T& value) const {
		return find_impl(begin, m_data.size(), value);
	}
	ConstIt_t lower_bound(u32 begin, u32 count, const T& value) const {
		return find_impl(begin, begin + count, value);
	}
	ConstIt_t lower_bound(ConstIt_t first, ConstIt_t last, const T& value) const {
		return std::lower_bound(first, last, value, cmp);
	}



	bool exist(const T& value) const {
		return validIt_impl(find_impl(value), value);
	}

	ConstIt_t insert(const T& value) {
		It_t it = find_impl(value);
		return m_data.insert(it, value);
	}
	ConstIt_t insert(T&& value) {
		It_t it = find_impl(value);
		return m_data.insert(it, std::move(value));
	}

	/**
	 * @note This is not a true in-place construction. A temporary object is
	 *       created to find the correct insertion position, and then the element is
	 *       move-inserted.
	 */
	template <class... Args>
	ConstIt_t insertEmplace(Args&&... args) {
		return insert(T(std::forward<Args>(args)...));
	}

	ConstIt_t erase(ConstIt_t pos) {
		return m_data.erase(pos);
	}
	ConstIt_t erase(ConstIt_t first, ConstIt_t last) {
		return m_data.erase(first, last);
	}
	// @return return the count of the erased elements
	size_t erase(const T& value) {
		auto range = std::equal_range(m_data.begin(), m_data.end(), value, cmp);

		size_t count = std::distance(range.first, range.second);
		if (count > 0) { m_data.erase(range.first, range.second); }

		return count;
	}
	// @return the count of element erased
	size_t erase(std::span<const T> values) {
		std::vector<T> valuesSorted(values.begin(), values.end());
		std::sort(valuesSorted.begin(), valuesSorted.end(), cmp);
		size_t old_size = m_data.size();
		m_data.erase(
		    tx::remove_multi_sorted(
		        m_data.begin(),
		        m_data.end(),
		        valuesSorted.begin(),
		        valuesSorted.end(),
		        cmp),
		    m_data.end());
		return old_size - m_data.size();
	}
	size_t erase(std::initializer_list<T> values) {
		return erase(std::span<const T>(values.begin(), values.size()));
	}
	size_t erase(const SortedArr& other) {
		size_t old_size = m_data.size();
		m_data.erase(
		    tx::remove_multi_sorted(
		        m_data.begin(), m_data.end(),
		        other.begin(), other.end(),
		        cmp),
		    m_data.end());
		return old_size - m_data.size();
	}


	void merge(const SortedArr& other) {
		size_t original_size = m_data.size();
		m_data.insert(m_data.end(), other.begin(), other.end());
		std::inplace_merge(m_data.begin(), m_data.begin() + original_size, m_data.end(), cmp);
	}
	void merge(SortedArr&& other) {
		// Choose the vector with the larger capacity to minimize memory allocations
		if (other.m_data.capacity() > m_data.capacity()) {
			size_t other_original_size = other.m_data.size();
			other.m_data.insert(other.m_data.end(), std::make_move_iterator(m_data.begin()), std::make_move_iterator(m_data.end()));
			std::inplace_merge(other.m_data.begin(), other.m_data.begin() + other_original_size, other.m_data.end(), cmp);
			m_data = std::move(other.m_data);
		} else {
			size_t original_size = m_data.size();
			m_data.insert(m_data.end(), std::make_move_iterator(other.m_data.begin()), std::make_move_iterator(other.m_data.end()));
			std::inplace_merge(m_data.begin(), m_data.begin() + original_size, m_data.end(), cmp);
			other.m_data.clear();
		}
	}
	void merge(std::span<const T> other) {
		size_t original_size = m_data.size();
		m_data.insert(m_data.end(), other.begin(), other.end());
		mergeTrailer(original_size);
	}
	void merge(std::initializer_list<T> ilist) {
		size_t original_size = m_data.size();
		m_data.insert(m_data.end(), ilist.begin(), ilist.end());
		mergeTrailer(original_size);
	}

	struct Inserter {
		SortedArr<T>* m_parent;
		Inserter(SortedArr<T>* parent) : m_parent(parent) {}
		void insert(const T& val) { m_parent->m_data.push_back(val); }
		void insert(T&& val) { m_parent->m_data.push_back(std::move(val)); }
	};
	template <std::invocable<Inserter&> Func>
	void insertMulti(Func&& f) {
		u32 original_size = m_data.size();
		Inserter inserter{ this };
		f(inserter);
		mergeTrailer(original_size);
	}

	void unique() {
		m_data.erase(std::unique(m_data.begin(), m_data.end()), m_data.end());
	}


	u32 index(ConstIt_t it) const { return index(m_data.begin(), it); }

	void sort() { sort_impl(); }


private:
	std::vector<T> m_data;
	CmpFunc cmp;

	void sort_impl(u32 offset = 0) {
		std::sort(m_data.begin() + offset, m_data.end(), cmp);
	}

	bool isSame_impl(const T& a, const T& b) const { return !cmp(a, b) && !cmp(b, a); }
	template <class ItT>
	bool validIt_impl(const ItT& it, const T& val) const { return it != m_data.end() && isSame_impl(*it, val); }


	ConstIt_t find_impl(const T& value) const { return find_impl(0, m_data.size(), value); }
	ConstIt_t find_impl(u32 begin, u32 end, const T& value) const {
		return std::lower_bound(m_data.cbegin() + begin, m_data.cbegin() + end, value, cmp);
	}
	It_t find_impl(const T& value) { return find_impl(0, m_data.size(), value); }
	It_t find_impl(u32 begin, u32 end, const T& value) {
		return std::lower_bound(m_data.begin() + begin, m_data.begin() + end, value, cmp);
	}

	void mergeTrailer(u32 trailerBegin) {
		sort_impl(trailerBegin);
		std::inplace_merge(m_data.begin(), m_data.begin() + trailerBegin, m_data.end(), cmp);
	}
};
template <class T>
using SortedArrInserter = SortedArr<T>::Inserter;


} // namespace tx