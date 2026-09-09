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

public:
	using StateStorage = impl::Storage<State_impl>;

public:
	using value_type = T;

public:
	// Because internal implementation, the total available capacity of meta
	// is one less then the capacity provided, meaning the actual count of
	// maximum partitions is `metaSize - 1`, therefore allocate
	// `desired_size + 1` meta buffer to meet expected buffer size.
	PackedPartedArrayOverlay(
	    T* dataBufferPtr, u32 dataBufferSize,
	    u32* metaBufferPtr, u32 metaBufferSize,
	    StateStorage* statePtr)
	    : PackedPartedArrayOverlay<T>(
	          statePtr, dataBufferPtr, dataBufferSize, metaBufferPtr, metaBufferSize) {
		m_meta[0] = 0;
		std::construct_at(m_state);
	}
	PackedPartedArrayOverlay(
	    std::span<T> dataBuffer, std::span<u32> metaBuffer, StateStorage* statePtr)
	    : PackedPartedArrayOverlay(dataBuffer.data(), dataBuffer.size(),
	                               metaBuffer.data(), metaBuffer.size(),
	                               statePtr) {}
	/**
	 * Default Constructor that produces an object in null state
	 */
	PackedPartedArrayOverlay()
	    : m_data(nullptr), m_meta(nullptr), m_state(nullptr),
	      m_capacity(0), m_metaCapacity(0) {}
	~PackedPartedArrayOverlay() = default;

	PackedPartedArrayOverlay(const PackedPartedArrayOverlay&) = default;
	PackedPartedArrayOverlay& operator=(const PackedPartedArrayOverlay&) = default;
	PackedPartedArrayOverlay(PackedPartedArrayOverlay&& other) = default;
	PackedPartedArrayOverlay& operator=(PackedPartedArrayOverlay&& other) = default;

	static PackedPartedArrayOverlay<T> fromExistingState(
	    T* dataBufferPtr, u32 dataBufferSize,
	    u32* metaBufferPtr, u32 metaBufferSize,
	    StateStorage* statePtr) {
		PackedPartedArrayOverlay<T> obj(
		    statePtr, dataBufferPtr, dataBufferSize, metaBufferPtr, metaBufferSize);
		obj.m_state = std::launder(obj.m_state);
		return obj;
	}
	static PackedPartedArrayOverlay<T> fromExistingState(
	    std::span<T> dataBuffer, std::span<u32> metaBuffer,
	    StateStorage* statePtr) {
		PackedPartedArrayOverlay<T> obj(
		    statePtr,
		    dataBuffer.data(), dataBuffer.size(),
		    metaBuffer.data(), metaBuffer.size());
		obj.m_state = std::launder(obj.m_state);
		return obj;
	}

	bool valid() const {
		return m_data && m_meta && m_capacity && m_metaCapacity && m_state;
	}

public:
	// ################ Partition Proxies ################

	using ConstPartition = std::span<const T>;
	using Partition = std::span<T>;

	// BackPartition is a handle to the current back partition, and any
	// operation changing the partition structure invalidates its old meaning.
	// It always points to the last partition.
	struct BackPartition : std::ranges::view_interface<BackPartition> {
		friend PackedPartedArrayOverlay<T>;

		using iterator = typename std::span<T>::iterator;
		using const_iterator = typename std::span<T>::const_iterator;

		/**
		 * Since BackPartition have to be mutable, or else it's just going to
		 * be ConstParitition anyways, therefore this class is designed under
		 * the assumption that the object of this class will always be mutable.
		 */

		/**
		 * Since BackPartition can only be constructed by factory function
		 * provided by PackedPartedArrayOverlay, given it's only construct is
		 * private, and m_parent is never mutated during the lifetime of
		 * BackPartition, it is safe to assume that m_parent is always a valid
		 * pointer during the lifetime of BackPartition.
		 */

	public:
		// default constructor is delibrately removed. only copy constructors
		// are available, and only way to obtain a object of this class is by
		// tx::PackedPartedArrayOverlay::backPartition(). Think of this like
		// std::vector::iterator.

		// ================ Viewers ================

		std::span<T> span() const {
			impl::assert_impl(impl::assert::overlay_object_valid(m_parent));
			IndexRange range = range_impl();
			return std::span<T>(m_parent->m_data + range.offset, range.size);
		}
		operator std::span<T>() const { return span(); }

		std::span<T> first(u32 count) const { return span().first(count); }
		std::span<T> last(u32 count) const { return span().last(count); }
		std::span<T> subspan(u32 offset, u32 count = InvalidU32)
		    const { return span().subspan(offset, count); }

		iterator begin() const { return span().begin(); }
		iterator end() const { return span().end(); }

		u32 capacity() const {
			impl::assert_impl(impl::assert::overlay_object_valid(m_parent));
			return m_parent->capacity_elements() - range_impl().offset;
		}

		// ================ Modifiers ================

		template <class... Args>
		void emplace_back(Args&&... args) {
			impl::assert_impl(impl::assert::overlay_object_valid(m_parent));
			impl::assert_impl(impl::assert::bad_expansion(this->size(), this->capacity()));
			std::construct_at(push_impl(), std::forward<Args>(args)...);
		}

		void push_back(const T& val) { emplace_back(val); }
		void push_back(T&& val) { emplace_back(std::move(val)); }

		template <std::random_access_iterator It>
		iterator push_back(It first, It last) {
			impl::assert_impl(impl::assert::overlay_object_valid(m_parent));
			u32 count = static_cast<u32>(std::distance(first, last));
			impl::assert_impl(impl::assert::bad_expansion(
			    this->size(), this->capacity(), count));
			u32 originalEnd = range_impl().end();
			std::uninitialized_copy(first, last, push_impl(count));
			return begin() + originalEnd;
		}
		iterator push_back(std::span<const T> data) {
			return push_back(data.begin(), data.end());
		}

		void pop_back() {
			impl::assert_impl(impl::assert::overlay_object_valid(m_parent));
			impl::assert_impl(impl::assert::buffer_not_empty(this->size()));
			u32 head = m_parent->m_state->metaSize - 1;
			m_parent->m_meta[head]--;
			std::destroy_at(m_parent->m_data + m_parent->m_meta[head]);
		}

		void clear() {
			impl::assert_impl(impl::assert::overlay_object_valid(m_parent));
			IndexRange range = range_impl();
			if (range.size == 0) return;

			std::destroy(m_parent->m_data + range.offset,
			             m_parent->m_data + range.offset + range.size);

			m_parent->m_meta[m_parent->m_state->metaSize - 1] = range.offset;
		}

		iterator erase(const_iterator first, const_iterator last) {
			impl::assert_impl(impl::assert::overlay_object_valid(m_parent));
			u32 eraseOffset = itIndex_impl(first);
			u32 eraseCount = static_cast<u32>(std::distance(first, last));

			if (eraseCount == 0)
				return begin() + eraseOffset;

			IndexRange range = range_impl();
			impl::assert_impl(impl::assert::out_of_range(
			    eraseOffset + eraseCount - 1, range.size));

			T* partBegin = m_parent->m_data + range.offset;
			T* partEnd = m_parent->m_data + range.end();
			T* erasePtr = partBegin + eraseOffset;

			std::move(erasePtr + eraseCount, partEnd, erasePtr);
			std::destroy(partEnd - eraseCount, partEnd);

			m_parent->m_meta[m_parent->m_state->metaSize - 1] -= eraseCount;

			return begin() + eraseOffset;
		}
		iterator erase(const_iterator pos) {
			return erase(pos, pos + 1);
		}

		template <class... Args>
		iterator emplace(const_iterator pos, Args&&... args) {
			impl::assert_impl(impl::assert::overlay_object_valid(m_parent));
			if (pos == this->cend()) {
				u32 originalEnd = range_impl().end();
				emplace_back(std::forward<Args>(args)...);
				return begin() + originalEnd;
			}
			impl::assert_impl(impl::assert::bad_expansion(this->size(), this->capacity()));
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
			impl::assert_impl(impl::assert::overlay_object_valid(m_parent));
			if (pos == this->cend()) return push_back(first, last);
			u32 index = itIndex_impl(pos);
			u32 count = static_cast<u32>(std::distance(first, last));
			if (count == 0) return begin() + index;
			impl::assert_impl(impl::assert::bad_expansion(
			    this->size(), this->capacity(), count));

			auto [ptr, uninitMemSize] = makeSpace_impl(index, count);
			std::copy_n(first, count - uninitMemSize, ptr);
			std::uninitialized_copy(last - uninitMemSize, last, ptr + count - uninitMemSize);
			return begin() + index;
		}

	private:
		PackedPartedArrayOverlay<T>* m_parent;

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

	u32 size_elements() const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		return m_meta[m_state->metaSize - 1];
	}
	u32 size_partitions() const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		return m_state->metaSize - 1;
	}
	u32 size_meta() const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		return m_state->metaSize;
	}
	u32 capacity_elements() const { return m_capacity; }
	u32 capacity_partitions() const { return m_metaCapacity - 1; }
	u32 capacity_meta() const { return m_metaCapacity; }

	bool empty() const { return !size_partitions(); }
	operator bool() const { return !empty(); }

	// data of elements
	T* data() const { return m_data; }
	// user don't suppose to access m_meta buffer directly

	ConstPartition front() const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_empty(size_partitions()));
		return at_impl(0);
	}
	Partition front() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_empty(size_partitions()));
		return at_impl(0);
	}
	ConstPartition back() const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_empty(size_partitions()));
		return at_impl(m_state->metaSize - 2);
	}
	Partition back() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_empty(size_partitions()));
		return at_impl(m_state->metaSize - 2);
	}

	// explicitly request an expandable partition
	BackPartition backPartition() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_empty(size_partitions()));
		return BackPartition(this);
	}

	ConstPartition operator[](u32 index) const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::out_of_range(size_partitions(), index));
		return at_impl(index);
	}
	Partition operator[](u32 index) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::out_of_range(size_partitions(), index));
		return at_impl(index);
	}

	// ################ Modifiers ################

	BackPartition push_back() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::bad_expansion(m_state->metaSize, m_metaCapacity));
		m_meta[m_state->metaSize] = m_meta[m_state->metaSize - 1];
		m_state->metaSize++;
		return BackPartition(this);
	}
	// forwarding the arguments to BackPartition::push_back()
	template <class... Args>
	    requires requires(BackPartition& p, Args&&... args) {
		    p.push_back(std::forward<Args>(args)...);
	    } && (sizeof...(Args) > 0)
	BackPartition push_back(Args&&... args) {
		push_back().push_back(std::forward<Args>(args)...);
		return BackPartition(this);
	}

	void pop_back() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::buffer_not_empty(size_partitions()));
		std::destroy(m_data + m_meta[m_state->metaSize - 2], m_data + m_meta[m_state->metaSize - 1]);
		m_state->metaSize--;
	}
	void clear() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		std::destroy(m_data, m_data + m_meta[m_state->metaSize - 1]);
		m_state->metaSize = 1;
		m_meta[0] = 0;
	}

	// Destroies internal state object, ends lifetime of this overlay and
	// every other overlays that share the same buffers.
	// This is the point of no return.
	void destruct() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		std::destroy_at(m_state);
	}

private:
	T* m_data = nullptr;
	u32* m_meta = nullptr;
	State_impl* m_state = nullptr;
	u32 m_capacity = 0;
	u32 m_metaCapacity = 0;

private:
	// ################ Helpers ################

	PackedPartedArrayOverlay(
	    StateStorage* statePtr,
	    T* dataBufferPtr, u32 dataBufferSize,
	    u32* metaBufferPtr, u32 metaBufferSize)
	    : m_data(dataBufferPtr), m_meta(metaBufferPtr),
	      m_state(reinterpret_cast<State_impl*>(statePtr)),
	      m_capacity(dataBufferSize), m_metaCapacity(metaBufferSize) {
		impl::assert_impl(
		    [this] { return this->valid(); },
		    [=] {
			    return std::format(
			        "Bad construction. Invalid pointer or size provided."
			        " dataBufferPtr = {};"
			        " metaBufferPtr = {};"
			        " statePtr = {};"
			        " dataBufferSize = {};"
			        " metaBufferSize = {}",
			        static_cast<const void*>(dataBufferPtr),
			        static_cast<const void*>(metaBufferPtr),
			        static_cast<const void*>(statePtr),
			        dataBufferSize,
			        metaBufferSize);
		    });
	}

	/**
	 * Each element of m_meta is the `offset` / begin of a partition.
	 * The last element of m_meta is equivalent with m_size: the total count of
	 * elements in the buffer. It also represents the end of the last partition.
	 * Therefore, the range of every partition can be calculated with the same
	 * algorithm (at_impl) without edge cases.
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
	~PackedPartedArrayOverlayInlined() { this->destruct(); }
	typename PackedPartedArrayOverlay<T>::StateStorage m_metaStorage;
	PackedPartedArrayOverlayInlined(const PackedPartedArrayOverlayInlined&) = delete;
	PackedPartedArrayOverlayInlined& operator=(const PackedPartedArrayOverlayInlined&) = delete;
	PackedPartedArrayOverlayInlined(PackedPartedArrayOverlayInlined&& other) = delete;
	PackedPartedArrayOverlayInlined& operator=(PackedPartedArrayOverlayInlined&& other) = delete;
};
} // namespace tx