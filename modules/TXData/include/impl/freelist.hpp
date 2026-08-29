// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/data_utils.hpp"
#include "tx/basic_types.hpp"
#include "tx/exception.hpp"
#include <concepts>

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
	    : m_data(bufferPtr),
	      m_state(std::construct_at(reinterpret_cast<State_impl*>(statePtr))),
	      m_dataSize(bufferSize) {
	}

public:
	SizeT allocate() {
		if (m_state->freeCount == 0) { // no free slot, bump
			impl::assert_impl([&]() { return m_state->size < m_dataSize; },
			                  "tx::FreelistOverlay::free: Size overflowed capacity.");
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
		impl::assert_impl([&]() { return index < m_state->size; },
		                  "tx::FreelistOverlay::free: Invalid argument. Index out of range.");

		// index at back, pop
		if (index == m_state->size - 1) {
			m_state->size--;
			return;
		}

		m_state->freeCount++;
		std::construct_at(metaAt_impl(index), m_state->freeHead);
		m_state->freeHead = index;
	}

	SizeT size() const { return m_state->size - m_state->freeCount; }
	SizeT capacity() const { return m_dataSize; }
	bool empty() const { return !size(); }
	bool full() const { return size() == capacity(); }

private:
	T* m_data;
	State_impl* m_state;
	SizeT m_dataSize;

	SizeT* metaAt_impl(SizeT index) {
		return reinterpret_cast<SizeT*>(m_data + index);
	}
};

} // namespace tx