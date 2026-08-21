// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include "tx/algorithm.hpp"
#include <span>
#include <algorithm>
#include <type_traits>
#include <concepts>

#include <vector> // DevNote: replace with tx::BasicStorage

namespace tx {

// DevNote: move to data_utils.hpp
template <class T, std::invocable<T, T> CompareFunc>
inline bool isSame(const T& a, const T& b, CompareFunc&& cmp) {
	return !cmp(a, b) && !cmp(b, a);
}




/**
 * An semi-overlay class that is design to be light weight, and for user to
 * query the location of an element faster then O(N)
 * This class is a set of 4 variations that performs the same task, produced by
 * the 2 template arguments:
 * - keepOriginalData
 *   - if true, a copy would be made from the given data, and the original data
 *     will not be touched
 *   - if false, the given data will be sorted inplace, and the order of it is
 *     expected to be changed unless it's already sorted
 * - keepOriginalIndex
 *   - if true, a new array of indices will be allocated. It will store the
 *     original index of each element.
 * This class is encouraged to be used with SoA structure
 */
template <class T, bool keepOriginalData, bool keepOriginalIndex, class CompareFunc = std::less<>>
    requires std::is_invocable_r_v<bool, CompareFunc, T, T>
class BinarySetView {
	// *Alternative of the deprecated: tx::KVMap and tx::SetView*
private:
	using It_t = typename std::span<T>::iterator;
	using ConstIt_t = typename std::span<T>::const_iterator;

public:
	using iterator = It_t;
	using const_iterator = ConstIt_t;

	using value_type = T;

public:
	/**
	 * The provided data will be sorted during construction of this object
	 */
	BinarySetView(std::span<std::conditional_t<keepOriginalData, const T, T>> data,
	              CompareFunc cmpFunc = std::less<>{})
	    : m_cmp(std::move(cmpFunc)) {
		if constexpr (keepOriginalData) {
			m_dataCopied = std::vector<T>(data.begin(), data.end());
			m_data = std::span<T>{ m_dataCopied };
		} else {
			m_data = data;
		}

		if constexpr (keepOriginalIndex) {
			// create index array
			m_originalIndex = std::vector<u32>(data.size());
			for (u32 i = 0; i < data.size(); i++) {
				m_originalIndex[i] = i;
			}

			// multi sort
			tx::sort_multi(m_data.begin(), m_data.end(), m_cmp, m_originalIndex.begin());
		} else {
			std::sort(m_data.begin(), m_data.end(), m_cmp);
		}
	}
	BinarySetView() = default;

	// ################ Move/Copy Semantics Boilerplate ################

	BinarySetView(const BinarySetView& other)
	    : m_dataCopied(other.m_dataCopied),
	      m_originalIndex(other.m_originalIndex),
	      m_cmp(other.m_cmp) {
		if constexpr (keepOriginalData) {
			// Point m_data to OUR NEW copied vector buffer
			m_data = std::span<T>(m_dataCopied);
		} else {
			// Non-owning view: copy the span pointer as-is
			m_data = other.m_data;
		}
	}
	BinarySetView& operator=(const BinarySetView& other) {
		if (this != &other) {
			m_dataCopied = other.m_dataCopied;
			m_originalIndex = other.m_originalIndex;
			m_cmp = other.m_cmp;

			if constexpr (keepOriginalData) {
				m_data = std::span<T>(m_dataCopied);
			} else {
				m_data = other.m_data;
			}
		}
		return *this;
	}
	BinarySetView(BinarySetView&& other) noexcept
	    : m_dataCopied(std::move(other.m_dataCopied)),
	      m_originalIndex(std::move(other.m_originalIndex)),
	      m_cmp(std::move(other.m_cmp)) {
		if constexpr (keepOriginalData) {
			m_data = std::span<T>(m_dataCopied);
		} else {
			m_data = other.m_data;
		}
		other.m_data = {};
	}
	BinarySetView& operator=(BinarySetView&& other) noexcept {
		if (this != &other) {
			m_dataCopied = std::move(other.m_dataCopied);
			m_originalIndex = std::move(other.m_originalIndex);
			m_cmp = std::move(other.m_cmp);

			if constexpr (keepOriginalData) {
				m_data = std::span<T>(m_dataCopied);
			} else {
				m_data = other.m_data;
			}
			other.m_data = {};
		}
		return *this;
	}

	// ################ Query ################

	u32 size() const { return m_data.size(); }
	bool empty() const { return m_data.empty(); }
	T& atIndex(u32 index) { return m_data[index]; }
	const T& atIndex(u32 index) const { return m_data[index]; }
	// DevNote: add bound check

	bool exist(const T& val) const {
		auto it = find_impl(val);
		return validIt_impl(it) && isSame_impl(*it, val);
	}
	/**
	 * @return index of the argument @a val
	 * if `keepOriginalIndex` is true, then the returned index will be the
	 * index of the element in the original array instead of the sorted array
	 */
	u32 find(const T& val) const {
		auto it = find_impl(val);
		if (validIt_impl(it) && isSame_impl(*it, val)) {
			u32 currentIndex = static_cast<u32>(it - m_data.begin());
			if constexpr (keepOriginalIndex) {
				return m_originalIndex[currentIndex];
			} else {
				return currentIndex;
			}
		}
		return InvalidU32;
	}

private:
	std::span<T> m_data;
	[[no_unique_address]] std::conditional_t<keepOriginalData, std::vector<T>, Nothing> m_dataCopied;
	[[no_unique_address]] std::conditional_t<keepOriginalIndex, std::vector<u32>, Nothing> m_originalIndex;
	CompareFunc m_cmp;

	It_t find_impl(const T& val) {
		return std::lower_bound(m_data.begin(), m_data.end(), val, m_cmp);
	}
	bool isSame_impl(const T& vala, const T& valb) const { return isSame(vala, valb, m_cmp); }
	bool validIt_impl(It_t it) const { return it != m_data.end(); }
};

// keepOriginalData=false -> non-owning span<T>
template <class T, class CompareFunc>
BinarySetView(std::span<T>, CompareFunc) -> BinarySetView<T, false, false, CompareFunc>;

template <class T, class CompareFunc>
BinarySetView(std::span<T>, CompareFunc) -> BinarySetView<T, false, true, CompareFunc>;

// keepOriginalData=true -> owning copy, span<const T>
template <class T, class CompareFunc>
BinarySetView(std::span<const T>, CompareFunc) -> BinarySetView<T, true, false, CompareFunc>;

template <class T, class CompareFunc>
BinarySetView(std::span<const T>, CompareFunc) -> BinarySetView<T, true, true, CompareFunc>;

template <class T, class CompareFunc = std::less<>>
    requires std::is_invocable_r_v<bool, CompareFunc, T, T>
using InplaceBinarySetView = BinarySetView<T, false, false, CompareFunc>;

template <class T, class CompareFunc = std::less<>>
    requires std::is_invocable_r_v<bool, CompareFunc, T, T>
using IndexedInplaceBinarySetView = BinarySetView<T, false, true, CompareFunc>;

template <class T, class CompareFunc = std::less<>>
    requires std::is_invocable_r_v<bool, CompareFunc, T, T>
using DetachedBinarySetView = BinarySetView<T, true, false, CompareFunc>;

template <class T, class CompareFunc = std::less<>>
    requires std::is_invocable_r_v<bool, CompareFunc, T, T>
using IndexedDetachedBinarySetView = BinarySetView<T, true, false, CompareFunc>;
template <class T, class CompareFunc = std::less<>>
    requires std::is_invocable_r_v<bool, CompareFunc, T, T>
using MappedBinarySetView = BinarySetView<T, true, true, CompareFunc>;
} // namespace tx