// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "tx/basic_types.hpp"
#include "impl/data_utils.hpp"
#include "tx/exception.hpp"
#include <algorithm>
#include <iterator>
#include <cstring>
#include <memory>
#include <ranges>
#include <span>

namespace tx {
// Packed Partitioned Array, made for building a write once, read many buffer
// All partitions are packed together without gap, providing maximum memory
// efficiency and cache locality
template <class T>
class PackedPartedArrayOverlay {
	/**
	 * The size of each partition that is not at the back is unable to change.
	 * Only the partition at the back (called BackPartition) can be freely
	 * modified like a normal std::vector.
	 * 
	 * All the accessor functions works across all partitions, returning
	 * uniform results: Partition/ConstPartition. To obtain a size-mutable
	 * partition proxy, use backPartition().
	 * 
	 * A new constructed object of this class is unable to do anything except
	 * create a new partition. Having at least one partition is required for
	 * anything to be written on the data buffer.
	 */
private:
	struct State_impl {
		u32 metaSize = 1;
	};

	inline static constexpr const u32 StateStorageSize = sizeof(State_impl);
	inline static constexpr const u32 StateStorageAlign = alignof(State_impl);

public:
	struct alignas(StateStorageAlign) StateStorage {
	private:
		std::byte data[StateStorageSize];
	};

public:
	using value_type = T;

public:
	// Because internal implementation, the total available capacity of meta
	// is one less then the capacity provided, meaning that the actual count of
	// maximum partitions is `metaSize - 1`, therefore allocate
	// `desired_size + 1` meta buffer to meet expected buffer size.
	PackedPartedArrayOverlay(T* data, u32 size, u32* metaData, u32 metaSize, StateStorage* statePtr)
	    : m_data(data), m_capacity(size),
	      m_meta(metaData), m_metaCapacity(metaSize),
	      m_state(std::construct_at(reinterpret_cast<State_impl*>(statePtr))) {
		m_meta[0] = 0;
	}
	PackedPartedArrayOverlay() = default;

	using ConstPartition = std::span<const T>;
	using Partition = std::span<T>;

	// BackPartition is a handle to the current back partition, and any
	// operation changing the partition structure invalidates its old meaning.
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
		// default constructor is delibrately removed. only copy constructors
		// are available, and only way to obtain a object of this class is by
		// tx::PackedPartedArrayOverlay::backPartition(). Think of this like
		// std::vector::iterator.

		// ================ Viewers ================

		std::span<T> span() const {
			if (!m_parent) return {};
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

		u32 capacity() const { return m_parent->capacity_elements() - range_impl().offset; }

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
			impl::assert_impl([&]() { return this->size() > 0; },
			                  "tx::PackedPartedArrayOverlay::BackPartition::pop_back(): Called on empty partition.");
			u32 head = m_parent->m_state->metaSize - 1;
			m_parent->m_meta[head]--;
			std::destroy_at(m_parent->m_data + m_parent->m_meta[head]);
		}

		void clear() {
			IndexRange range = range_impl();
			if (range.size == 0) return;

			std::destroy(m_parent->m_data + range.offset,
			             m_parent->m_data + range.offset + range.size);

			m_parent->m_meta[m_parent->m_state->metaSize - 1] = range.offset;
		}

		iterator erase(const_iterator pos) {
			return erase(pos, pos + 1);
		}
		iterator erase(const_iterator first, const_iterator last) {
			u32 eraseOffset = itIndex_impl(first);
			u32 eraseCount = static_cast<u32>(std::distance(first, last));

			if (eraseCount == 0)
				return begin() + eraseOffset;

			IndexRange range = range_impl();
			impl::assert_impl([&]() { return eraseOffset + eraseCount <= range.size; },
			                  "tx::PackedPartedArrayOverlay::BackPartition::erase(): Subscript out of range.");

			T* partBegin = m_parent->m_data + range.offset;
			T* partEnd = m_parent->m_data + range.end();
			T* erasePtr = partBegin + eraseOffset;

			std::move(erasePtr + eraseCount, partEnd, erasePtr);
			std::destroy(partEnd - eraseCount, partEnd);

			m_parent->m_meta[m_parent->m_state->metaSize - 1] -= eraseCount;

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

		IndexRange range_impl() const {
			u32 head = m_parent->m_state->metaSize - 1;
			u32 offset = m_parent->m_meta[head - 1];
			return IndexRange{ offset, m_parent->m_meta[head] - offset };
		}
		u32 itIndex_impl(const_iterator it) const { return findIteratorIndex(this->cbegin(), it); }

		T* push_impl(u32 count = 1) {
			u32 head = m_parent->m_state->metaSize - 1;
			T* ptr = m_parent->m_data + m_parent->m_meta[head];
			m_parent->m_meta[head] += count;
			return ptr;
		}

		// using STL's 2 phase move optimization
		// the space made available will still contain alive object instead of
		// uninitialized memory
		// @return `first`: the pointer to the first element made available
		//         `second`: the size of trailing uninitialized memory
		std::pair<T*, u32> makeSpace_impl(u32 begin, u32 size = 1) {
			IndexRange range = range_impl();
			T* eraseBegin = m_parent->m_data + range.begin() + begin;
			T* partEnd = m_parent->m_data + range.end();

			u32 uninitMemSize = 0;
			if (size >= partEnd - eraseBegin) {
				std::uninitialized_move(eraseBegin, partEnd, eraseBegin + size);
				uninitMemSize = eraseBegin + size - partEnd;
			} else {
				std::uninitialized_move(partEnd - size, partEnd, partEnd);
				std::move_backward(eraseBegin, partEnd - size, partEnd);
			}

			m_parent->m_meta[m_parent->m_state->metaSize - 1] += size;
			return { eraseBegin, uninitMemSize };
		}

	private:
		BackPartition(PackedPartedArrayOverlay<T>* parent)
		    : m_parent(parent) {}
	};

	// ################ Viewers ################

	u32 size_elements() const { return m_meta[m_state->metaSize - 1]; }
	u32 size_partitions() const { return m_state->metaSize - 1; }
	u32 size_meta() const { return m_state->metaSize; }
	u32 capacity_elements() const { return m_capacity; }
	u32 capacity_partitions() const { return m_metaCapacity - 1; }
	u32 capacity_meta() const { return m_metaCapacity; }

	bool empty() const { return !size_partitions(); }
	operator bool() const { return !empty(); }

	// data of elements
	T* data() const { return m_data; }
	// user don't suppose to access m_meta buffer directly

	ConstPartition front() const {
		impl::assert_impl([&]() { return m_state->metaSize >= 2; },
		                  "tx::PackedPartedArray::front(): Call on empty object.");
		return at_impl(0);
	}
	Partition front() {
		impl::assert_impl([&]() { return m_state->metaSize >= 2; },
		                  "tx::PackedPartedArray::front(): Call on empty object.");
		return at_impl(0);
	}
	ConstPartition back() const {
		impl::assert_impl([&]() { return m_state->metaSize >= 2; },
		                  "tx::PackedPartedArray::back(): Call on empty object.");
		return at_impl(m_state->metaSize - 2);
	}
	Partition back() {
		impl::assert_impl([&]() { return m_state->metaSize >= 2; },
		                  "tx::PackedPartedArray::back(): Call on empty object.");
		return at_impl(m_state->metaSize - 2);
	}

	// explicitly request an expandable partition
	BackPartition backPartition() {
		impl::assert_impl([&]() { return m_state->metaSize >= 2; },
		                  "tx::PackedPartedArray::backPartition(): Call on empty object.");
		return BackPartition(this);
	}

	ConstPartition operator[](u32 index) const {
		impl::assert_impl([&]() { return m_state->metaSize - 1 > index; },
		                  "tx::PackedPartedArray::operator[]: Subscript out of range.");
		return at_impl(index);
	}
	Partition operator[](u32 index) {
		impl::assert_impl([&]() { return m_state->metaSize - 1 > index; },
		                  "tx::PackedPartedArray::operator[]: Subscript out of range.");
		return at_impl(index);
	}

	// ################ Modifiers ################

	BackPartition push_back() {
		m_meta[m_state->metaSize] = m_meta[m_state->metaSize - 1];
		m_state->metaSize++;
		return BackPartition(this);
	}
	template <class... Args>
	    requires requires(BackPartition& p, Args&&... args) {
		    p.push_back(std::forward<Args>(args)...);
	    } && (sizeof...(Args) > 0)
	BackPartition push_back(Args&&... args) {
		push_back().push_back(std::forward<Args>(args)...);
		return BackPartition(this);
	}

	void pop_back() {
		impl::assert_impl([&]() { return m_state->metaSize >= 2; },
		                  "tx::PackedPartedArrayOverlay::pop_back(): Call on empty object.");
		std::destroy(m_data + m_meta[m_state->metaSize - 2], m_data + m_meta[m_state->metaSize - 1]);
		m_state->metaSize--;
	}
	void clear() {
		std::destroy(m_data, m_data + m_meta[m_state->metaSize - 1]);
		m_state->metaSize = 1;
		m_meta[0] = 0;
	}

	// Destroies internal state object, ends lifetime of this overlay and
	// every other overlays that share the same buffers.
	// This is the point of no return.
	void destruct() {
		std::destroy_at(m_state);
	}

	// ################ Relocation ################

	void relocateData(T* newData, u32 newCapacity = InvalidU32) {
		if (newCapacity == InvalidU32) newCapacity = m_capacity;
		assert_impl([&]() { return newCapacity >= size_elements(); },
		            "tx::PackedPartedArrayOverlay::relocateData(): new buffer too small.");

		if constexpr (std::is_trivially_copyable_v<T>) {
			std::memcpy(newData, m_data, size_elements() * sizeof(T));
		} else {
			std::uninitialized_move(m_data, m_data + m_meta[m_state->metaSize - 1], newData);
			std::destroy(m_data, m_data + m_meta[m_state->metaSize - 1]);
		}

		m_data = newData;
		m_capacity = newCapacity;
	}

	void relocateMeta(u32* newMeta, u32 newMetaCapacity = InvalidU32) {
		if (newMetaCapacity == InvalidU32) newMetaCapacity = m_metaCapacity;
		impl::assert_impl([&]() { return newMetaCapacity >= m_state->metaSize; },
		                  "tx::PackedPartedArrayOverlay::relocateMeta(): new buffer too small.");

		std::copy(m_meta, m_meta + m_state->metaSize, newMeta);

		m_meta = newMeta;
		m_metaCapacity = newMetaCapacity;
	}


private:
	T* m_data = nullptr;
	u32 m_capacity = 0;
	u32* m_meta = nullptr;
	u32 m_metaCapacity = 0;
	State_impl* m_state = nullptr;

	/**
	 * Each element of m_meta is the `offset` / begin of a partition.
	 * The last element of m_meta is equivalent with m_size: the total count of
	 * elements in the buffer. It also represents the end of the last partition.
	 * Therefore, the range of every partition can be calculated with the same
	 * algorithm (at_impl) without edge cases.
	 * 
	 */

	Partition at_impl(u32 index) const {
		return Partition(m_data + m_meta[index],
		                 m_data + m_meta[index + 1]);
	}

protected:
};

template <class T>
class PackedPartedArrayOverlayInlined : public PackedPartedArrayOverlay<T> {
public:
	PackedPartedArrayOverlayInlined(T* data, u32 size, u32* metaData, u32 metaSize)
	    : PackedPartedArrayOverlay<T>(data, size, metaData, metaSize, &m_metaStorage) {}
	typename PackedPartedArrayOverlay<T>::StateStorage m_metaStorage;
	PackedPartedArrayOverlayInlined(const PackedPartedArrayOverlayInlined&) = delete;
	PackedPartedArrayOverlayInlined& operator=(const PackedPartedArrayOverlayInlined&) = delete;
	PackedPartedArrayOverlayInlined(PackedPartedArrayOverlayInlined&& other) = delete;
	PackedPartedArrayOverlayInlined& operator=(PackedPartedArrayOverlayInlined&& other) = delete;
};
} // namespace tx