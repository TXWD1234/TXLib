// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/data_foundation.hpp"
#include "tx/basic_types.hpp"
#include "tx/exception.hpp"
#include <memory>
#include <concepts>
#include <format>

namespace tx {
// O(1) array deletion, alternative of swap-and-pop deletion pattern
// no extra bookkeeping memory required
// will decrease density of the buffer
template <class T, std::integral SizeT = u32>
    requires(sizeof(T) >= sizeof(SizeT)) && (alignof(T) >= alignof(SizeT))
class FreelistOverlay {
private:
	static constexpr SizeT InvalidSize = ~SizeT{ 0 };

	struct State_impl {
		SizeT freeHead = InvalidSize;
		SizeT freeCount = SizeT{ 0 };
		SizeT size = SizeT{ 0 };
	};

public:
	using StateStorage = impl::Storage<State_impl>;

public:
	FreelistOverlay(T* bufferPtr, SizeT bufferSize, StateStorage* statePtr)
	    : FreelistOverlay<T>(statePtr, bufferPtr, bufferSize) {
		std::construct_at(m_state);
	}
	FreelistOverlay(std::span<T> buffer, StateStorage* statePtr)
	    : FreelistOverlay<T>(buffer.data(), buffer.size(), statePtr) {}

	FreelistOverlay() : m_data(nullptr), m_state(nullptr), m_dataSize(0) {}
	~FreelistOverlay() {}

	FreelistOverlay(const FreelistOverlay&) = default;
	FreelistOverlay& operator=(const FreelistOverlay&) = default;
	FreelistOverlay(FreelistOverlay&& other) = default;
	FreelistOverlay& operator=(FreelistOverlay&& other) = default;

	static FreelistOverlay<T> fromExistingState(
	    T* bufferPtr, SizeT bufferSize, StateStorage* statePtr) {
		FreelistOverlay<T> obj(statePtr, bufferPtr, bufferSize);
		obj.m_state = std::launder(obj.m_state);
		return obj;
	}
	static FreelistOverlay<T> fromExistingState(
	    std::span<T> buffer, StateStorage* statePtr) {
		FreelistOverlay<T> obj(statePtr, buffer.data(), buffer.size());
		obj.m_state = std::launder(obj.m_state);
		return obj;
	}

	bool valid() const { return m_data && m_dataSize && m_state; }

	void destruct() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		std::destroy_at(m_state);
	}

public:
	SizeT allocate() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		if (m_state->freeCount == 0) { // no free slot, bump
			impl::assert_impl(impl::assert::bad_expansion(m_state->size, m_dataSize));
			return m_state->size++;
		} else {
			m_state->freeCount--;
			SizeT freeIndex = m_state->freeHead;
			m_state->freeHead = *std::launder(metaAt_impl(freeIndex));
			std::destroy_at(std::launder(metaAt_impl(freeIndex)));
			return freeIndex;
		}
	}
	// double free is UB
	// accessing freed slot is UB
	// object originally at the freeing slot must be already destroyed
	void free(SizeT index) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::out_of_range(m_state->size, index));

		// index at back, pop
		if (index == m_state->size - 1) {
			m_state->size--;
			return;
		}

		m_state->freeCount++;
		std::construct_at(metaAt_impl(index), m_state->freeHead);
		m_state->freeHead = index;
	}

	void clear() {
		m_state->freeHead = InvalidSize;
		m_state->freeCount = SizeT{ 0 };
		m_state->size = SizeT{ 0 };
	}

	SizeT size() const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		return m_state->size - m_state->freeCount;
	}
	SizeT capacity() const { return m_dataSize; }
	bool empty() const { return !size(); }
	bool full() const { return size() == capacity(); }

private:
	T* m_data;
	State_impl* m_state;
	SizeT m_dataSize;

	FreelistOverlay(StateStorage* statePtr, T* bufferPtr, SizeT bufferSize)
	    : m_data(bufferPtr),
	      m_state(reinterpret_cast<State_impl*>(statePtr)),
	      m_dataSize(bufferSize) {
		impl::assert_impl(
		    [this] { return this->valid(); },
		    [=] {
			    return std::format(
			        "Bad construction. Invalid pointer or size provided."
			        " bufferPtr = {}; bufferSize = {}; statePtr = {}",
			        static_cast<const void*>(bufferPtr), bufferSize,
			        static_cast<const void*>(statePtr));
		    });
	}

	SizeT* metaAt_impl(SizeT index) {
		// std::launder is intentinally missing because constuction also need
		// to use this function. std::launder is expected to be used around
		// this function when it's used for any operation other than
		// construction
		return reinterpret_cast<SizeT*>(m_data + index);
	}
};

} // namespace tx