// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "tx/basic_types.hpp"
#include "impl/data_utils.hpp"
#include "tx/exception.hpp"
#include <algorithm>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>

namespace tx {
template <class T>
class PackedPartedArrayOverlay {
public:
	using value_type = T;

public:
	PackedPartedArrayOverlay(T* ptr, u32 size, IndexRange* metaPtr, u32 metaSize)
	    : m_data(ptr), m_size(0), m_capacity(size), m_meta(metaPtr), m_metaSize(0), m_metaCapacity(metaSize) {
	}
	PackedPartedArrayOverlay() = default;

	using ConstPartition = std::span<const T>;
	using Partition = std::span<T>;

	struct BackPartition : std::ranges::view_interface<BackPartition> {
		friend PackedPartedArrayOverlay<T>;

		using iterator = typename std::span<T>::iterator;
		using const_iterator = typename std::span<T>::const_iterator;

		/**
		 * Since BackPartition have to be mutable, or else it's just going to
		 * be ConstParitition anyways, therefore this class is designed under
		 * the assumption that the object of this class will always be mutable.
		 */

	public:
		BackPartition() = default;

		// ================ Viewers ================

		std::span<T> span() const {
			if (!m_parent || m_partIndex == InvalidU32) return {};
			IndexRange range = range_impl();
			return std::span<T>(m_parent->m_data + range.offset, range.size);
		}
		operator std::span<T>() const { return span(); }

		std::span<T> first(u32 count) const { return span().first(count); }
		std::span<T> last(u32 count) const { return span().last(count); }
		std::span<T> subspan(u32 offset, u32 count = InvalidU32) const {
			return span().subspan(offset, count);
		}

		iterator begin() const { return span().begin(); }
		iterator end() const { return span().end(); }

		u32 capacity() const { return m_parent->capacity_elements(); }

		// ================ Modifiers ================

		template <class... Args>
		void emplace_back(Args&&... args) { std::construct_at(push_impl(), std::forward<Args>(args)...); }

		void push_back(const T& val) { emplace_back(val); }
		void push_back(T&& val) { emplace_back(std::move(val)); }

		iterator push_back(std::span<const T> data) {
			u32 originalEnd = range_impl().end();
			std::uninitialized_copy(data.begin(), data.end(), push_impl(data.size()));
			return begin() + originalEnd;
		}
		template <std::random_access_iterator It>
		iterator push_back(It first, It last) {
			u32 originalEnd = range_impl().end();
			std::uninitialized_copy(first, last, push_impl(static_cast<u32>(std::distance(first, last))));
			return begin() + originalEnd;
		}

		void pop_back() {
			assert_impl([&]() { return this->size() > 0; },
			            "tx::PackedPartedArrayOverlay::BackPartition::pop_back(): Called on empty partition.");
			m_parent->m_size--;
			IndexRange& range = range_impl();
			range.size--;
			std::destroy_at(m_parent->m_data + range.end());
		}

		void clear() {
			IndexRange& range = range_impl();
			if (range.size == 0) return;

			std::destroy(m_parent->m_data + range.offset,
			             m_parent->m_data + range.offset + range.size);

			m_parent->m_size -= range.size;
			range.size = 0;
		}

		iterator erase(const_iterator pos) {
			return erase(pos, pos + 1);
		}
		iterator erase(const_iterator first, const_iterator last) {
			u32 eraseOffset = itIndex_impl(first);
			u32 eraseCount = static_cast<u32>(std::distance(first, last));

			if (eraseCount == 0)
				return begin() + eraseOffset;

			IndexRange& range = range_impl();
			assert_impl([&]() { return eraseOffset + eraseCount <= range.size; },
			            "tx::PackedPartedArrayOverlay::BackPartition::erase(): Subscript out of range.");

			T* partBegin = m_parent->m_data + range.offset;
			T* partEnd = m_parent->m_data + range.end();
			T* erasePtr = partBegin + eraseOffset;

			std::move(erasePtr + eraseCount, partEnd, erasePtr);
			std::destroy(partEnd - eraseCount, partEnd);

			range.size -= eraseCount;
			m_parent->m_size -= eraseCount;

			return begin() + eraseOffset;
		}

		template <class... Args>
		iterator emplace(const_iterator pos, Args&&... args) {
			if (pos == this->cend()) {
				u32 originalEnd = range_impl().end();
				emplace_back(std::forward<Args>(args)...);
				return begin() + originalEnd;
			}
			T temp(std::forward<Args>(args)...);
			u32 index = itIndex_impl(pos);
			*makeSpace_impl(index).first = std::move(temp);
			return begin() + index;
		}

		iterator insert(const_iterator pos, const T& val) {
			return emplace(pos, val);
		}
		iterator insert(const_iterator pos, T&& val) {
			return emplace(pos, std::move(val));
		}

		template <std::random_access_iterator InputIt>
		iterator insert(const_iterator pos, InputIt first, InputIt last) {
			if (pos == this->cend()) return push_back(first, last);
			u32 index = itIndex_impl(pos);
			u32 count = static_cast<u32>(std::distance(first, last));
			if (count == 0) return begin() + index;

			auto [ptr, uninitMemSize] = makeSpace_impl(index, count);
			std::copy_n(first, count - uninitMemSize, ptr);
			std::uninitialized_copy(last - uninitMemSize, last, ptr + count - uninitMemSize);
			return begin() + index;
		}

	private:
		PackedPartedArrayOverlay<T>* m_parent = nullptr;
		u32 m_partIndex = InvalidU32;

		IndexRange& range_impl() { return m_parent->m_meta[m_partIndex]; }
		u32 itIndex_impl(const_iterator it) const { return findIteratorIndex(this->cbegin(), it); }

		T* push_impl(u32 count = 1) {
			IndexRange& range = range_impl();
			range.size += count;
			m_parent->m_size += count;
			return m_parent->m_data + range.end() - count;
		}

		// using STL's 2 phase move optimization
		// the space made available will still contain alive object instead of
		// uninitialized memory
		// @return `first`: the pointer to the first element made available
		//         `second`: the size of trailing uninitialized memory
		std::pair<T*, u32> makeSpace_impl(u32 begin, u32 size = 1) {
			IndexRange& range = range_impl();
			T* eraseBegin = m_parent->m_data + range.begin() + begin;
			T* eraseEnd = eraseBegin + size;
			T* partEnd = m_parent->m_data + range.end();

			u32 uninitMemSize = 0;
			if (size >= partEnd - eraseBegin) {
				std::uninitialized_move(eraseBegin, partEnd, eraseBegin + size);
				uninitMemSize = eraseBegin + size - partEnd;
			} else {
				std::uninitialized_move(partEnd - size, partEnd, partEnd);
				std::move_backward(eraseBegin, partEnd - size, partEnd);
			}

			m_parent->m_size += size;
			range.size += size;
			return { eraseBegin, uninitMemSize };
		}

	private:
		BackPartition(PackedPartedArrayOverlay<T>* parent)
		    : m_parent(parent), m_partIndex(parent->m_metaSize - 1) {}
	};

	// ################ Viewers ################

	u32 size_elements() const { return m_size; }
	u32 size_partitions() const { return m_metaSize; }
	u32 capacity_elements() const { return m_capacity; }
	u32 capacity_partitions() const { return m_metaCapacity; }

	bool empty() const { return !size_elements() && !size_partitions(); }
	operator bool() const { return !empty(); }

	// data of elements
	T* data() const { return m_data; }
	// user don't suppose to access meta buffer directly

	ConstPartition front() const {
		assert_impl([&]() { return m_metaSize >= 1; },
		            "tx::PackedPartedArray::front(): Call on empty object.");
		return at_impl(0);
	}
	Partition front() {
		assert_impl([&]() { return m_metaSize >= 1; },
		            "tx::PackedPartedArray::front(): Call on empty object.");
		return at_impl(0);
	}
	ConstPartition back() const {
		assert_impl([&]() { return m_metaSize >= 1; },
		            "tx::PackedPartedArray::back(): Call on empty object.");
		return at_impl(m_metaSize - 1);
	}
	// special function that returns an expandable partition
	BackPartition back() {
		assert_impl([&]() { return m_metaSize >= 1; },
		            "tx::PackedPartedArray::back(): Call on empty object.");
		return BackPartition(this);
	}

	ConstPartition operator[](u32 index) const {
		assert_impl([&]() { return m_metaSize > index; },
		            "tx::PackedPartedArray::operator[]: Subscript out of range.");
		return at_impl(index);
	}
	Partition operator[](u32 index) {
		assert_impl([&]() { return m_metaSize > index; },
		            "tx::PackedPartedArray::operator[]: Subscript out of range.");
		return at_impl(index);
	}

	// ################ Modifiers ################

	BackPartition push_back() {
		std::construct_at(m_meta + m_metaSize++, m_size, 0);
		return BackPartition(this);
	}
	template <class... Args>
	    requires requires(BackPartition& p, Args&&... args) {
		    p.push_back(std::forward<Args>(args)...);
	    }
	BackPartition push_back(Args&&... args) {
		std::construct_at(m_meta + m_metaSize++, m_size, 0);
		BackPartition(this).push_back(std::forward<Args>(args)...);
		return BackPartition(this);
	}

	void pop_back() {
		m_metaSize--;
		IndexRange range = m_meta[m_metaSize];
		m_size -= range.size;
		std::destroy(m_data + range.begin(), m_data + range.end());
		std::destroy_at(m_meta + m_metaSize);
	}
	void clear() {
		std::destroy(m_data, m_data + m_size);
		std::destroy(m_meta, m_meta + m_metaSize);
		m_size = 0;
		m_metaSize = 0;
	}


private:
	T* m_data = nullptr;
	u32 m_size = 0,
	    m_capacity = 0;
	IndexRange* m_meta = nullptr;
	u32 m_metaSize = 0,
	    m_metaCapacity = 0;

	Partition at_impl(u32 index) const {
		return Partition(m_data + m_meta[index].begin(),
		                 m_data + m_meta[index].end());
	}

protected:
};
} // namespace tx