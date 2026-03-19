// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXMap
// File: set_view.h

#pragma once
#include <algorithm>
#include <functional>
#include <vector>

namespace tx {
// any manipulation of the original vector will invalidate the SetView object
// you can use the validate function to validate the object, or construct a new object
template <class T, class CmpFunc = std::less<T>>
class SetView {
	using It_t = std::vector<T>::iterator;
	using ConstIt_t = std::vector<T>::const_iterator;

public:
	SetView(std::vector<T>& in_vec, size_t in_begin, size_t in_end, CmpFunc in_cmp = std::less<T>{})
	    : m_vec(&in_vec), m_begin(in_begin), m_end(in_end), cmp(std::move(in_cmp)) {
		sort_impl();
	}


	inline bool exist(const T& key) const { return validIt_impl(findIt_impl(key), key); }
	inline int count(const T& key) const {
		auto it = findIt_impl(key);
		int counter = 0;
		while (validIt_impl(it, key)) {
			++counter;
			++it;
		}
		return counter;
	}
	inline int find(const T& key) const {
		It_t it = findIt_impl(key);
		if (!validIt_impl(it, key)) return -1;
		return static_cast<int>(it - m_vec->begin());
	}


	inline void validate() { sort_impl(); }

	inline void push_back(const T& val) {
		It_t it = findIt_impl(val);
		m_vec->insert(it, val);
		m_end++;
	}
	// inline void appendRange(int amount = 1) {
	// 	m_end += amount;
	// 	sort_impl();
	// }




private:
	std::vector<T>* m_vec;
	size_t m_begin, m_end;
	CmpFunc cmp;

	inline It_t findIt_impl(const T& key) {
		return std::lower_bound(
		    m_vec->begin() + m_begin, m_vec->begin() + m_end, key,
		    [this](const T& element, const T& key) {
			    return this->cmp(element, key);
		    });
	}
	inline ConstIt_t findIt_impl(const T& key) const {
		return std::lower_bound(
		    m_vec->begin() + m_begin, m_vec->begin() + m_end, key,
		    [this](const T& element, const T& key) {
			    return this->cmp(element, key);
		    });
	}

	inline void sort_impl() {
		std::sort(m_vec->begin() + m_begin, m_vec->begin() + m_end, std::forward<CmpFunc>(cmp));
	}

	//inline bool isSame_impl(const T& a, const T& b) const { return cmp(a, b) == cmp(b, a); }
	inline bool isSame_impl(const T& a, const T& b) const { return !cmp(a, b) && !cmp(b, a); }
	template <class It>
	inline bool validIt_impl(const It& it, const T& key) const { return (it != m_vec->begin() + m_end && isSame_impl(*it, key)); }
};

template <class T>
SetView(std::vector<T>&, size_t, size_t) -> SetView<T>;
template <class T, class CmpFunc>
SetView(std::vector<T>&, size_t, size_t, CmpFunc) -> SetView<T, CmpFunc>;


} // namespace tx
