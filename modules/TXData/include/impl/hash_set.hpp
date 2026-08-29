// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/data_utils.hpp"
#include "impl/numeric_utils.hpp"
#include "tx/basic_types.hpp"
#include "tx/exception.hpp"
#include "tx/type_traits.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <concepts>

namespace tx {
namespace impl {

template <class T, bool Trivial>
class HashSetOverlayTraits;

template <class T>
class HashSetOverlayTraits<T, true> {
public:
	using Value_t = T;
	using ValueStorage_t = void;
};
template <class T>
class HashSetOverlayTraits<T, false> {
public:
	using Value_t = u32;
	using ValueStorage_t = T;
};

} // namespace impl


template <
    class T,
    tx::invocable_r<size_t, T> FuncHash = std::hash<T>,
    tx::invocable_r<bool, T, T> FuncEqual = std::equal_to<T>>
class HashSetOverlay {
private:
	// ################ Traits & Policy ################

	static constexpr bool Trivial =
	    std::is_trivially_move_assignable_v<T> &&
	    std::is_trivially_move_constructible_v<T>;

	using Traits = impl::HashSetOverlayTraits<T, Trivial>;
	friend Traits;

private:
	// ################ Internal Typedefs & Constants ################

	struct Entry_impl {
		static constexpr u32 HashSize = sizeof(u8) * impl::ByteSize * 3;
		u32 hash : HashSize;
		// dist being 0xFF indicates the entry is empty
		u32 dist : sizeof(u8) * impl::ByteSize = (u8)0xFF;
		Traits::Value_t value;
	};
	struct State_impl {
		u32 entryCount = 0;
		bool distOverflowing = false;
	};

	static constexpr f32 MaxLoadFactor = 0.8f;
	static constexpr u8 MaxDistVal = (u8)(0xFF - 1);

public:
	// ################ Object Lifetime ################

	using EntryStorage = impl::Storage<Entry_impl>;
	using StateStorage = impl::Storage<State_impl>;
	// storage object of the value T if it's none trivial
	using ValueStorage = Traits::ValueStorage_t;

	HashSetOverlay(
	    EntryStorage* bufferPtr, u32 bufferSize,
	    StateStorage* statePtr)
	    requires Trivial
	    : m_data(tx::isPowTwo(bufferSize) ? reinterpret_cast<u8*>(bufferPtr) : nullptr),
	      m_dataBufferSize(tx::isPowTwo(bufferSize) ? bufferSize * sizeof(Entry_impl) : 0),
	      m_state(std::construct_at(reinterpret_cast<State_impl*>(statePtr))) {
		impl::assert_impl([&]() { return valid(); },
		                  "tx::HashSetOverlay::HashSetOverlay: Invalid object.");
		std::uninitialized_value_construct(
		    reinterpret_cast<Entry_impl*>(bufferPtr),
		    reinterpret_cast<Entry_impl*>(bufferPtr + bufferSize));
	}

	HashSetOverlay(
	    EntryStorage* bufferPtr, u32 bufferSize,
	    ValueStorage* valueBufferPtr, u32 valueBufferSize,
	    StateStorage* statePtr)
	    requires(!Trivial)
	    : m_data(tx::isPowTwo(bufferSize) ? reinterpret_cast<u8*>(bufferPtr) : nullptr),
	      m_value(tx::isPowTwo(bufferSize) && (valueBufferSize == bufferSize) ? reinterpret_cast<u8*>(valueBufferPtr) : nullptr),
	      m_dataBufferSize(tx::isPowTwo(bufferSize) ? bufferSize * sizeof(Entry_impl) : 0),
	      m_valueBufferSize(tx::isPowTwo(bufferSize) && (valueBufferSize == bufferSize) ? valueBufferSize * sizeof(T) : 0),
	      m_state(std::construct_at(reinterpret_cast<State_impl*>(statePtr))) {
		impl::assert_impl([&]() { return valid(); },
		                  "tx::HashSetOverlay::HashSetOverlay: Invalid object.");
		std::uninitialized_value_construct(
		    reinterpret_cast<Entry_impl*>(bufferPtr),
		    reinterpret_cast<Entry_impl*>(bufferPtr + bufferSize));
	}

public:
	// ################ Public Interface ################

	// @return the handle of the inserted entry. It will be invalidated after
	//         any modification
	template <class V>
	    requires std::constructible_from<T, V&&>
	u32 insert(V&& val) {
		if (full()) return InvalidU32;
		Entry_impl entry;
		if constexpr (Trivial) {
			entry.value = std::forward<V>(val);
		} else {
			entry.value = m_state->entryCount;
			std::construct_at(
			    reinterpret_cast<T*>(m_value + m_state->entryCount * sizeof(T)),
			    std::forward<V>(val));
		}
		m_state->entryCount++;
		return insert_impl(entry);
	}
	void erase() {
	}
	bool exist(const T& val) const { return find_impl(val) != InvalidU32; }
	u32 find(const T& val) const { return find_impl(val); }

	void clear() {}

	u32 size() { return m_state->entryCount; }
	u32 capacity() { return getEntryCapacity_impl() * MaxLoadFactor; }
	bool empty() { return !size(); }
	bool full() { return size() >= capacity() || m_state->distOverflowing; }

	template <class Self>
	decltype(auto) at(this Self&& self, u32 index) {
		impl::assert_impl([&]() { return index < self.getEntryCapacity_impl(); },
		                  "tx::HashSetOverlay::at(): Invalid argument. Index out of range.");
		return self.getValue_impl(self.dataAt_impl(index));
	}

	bool valid() const
	    requires Trivial
	{ return m_state && m_data && m_dataBufferSize; }
	bool valid() const
	    requires(!Trivial)
	{ return m_state && m_data && m_dataBufferSize && m_value && m_valueBufferSize; }

private:
	// ################ Runtime Data ################

	u8* m_data;
	[[no_unique_address]] std::conditional_t<
	    !Trivial, u8*, tx::Nothing> m_value;
	u32 m_dataBufferSize;
	[[no_unique_address]] std::conditional_t<
	    !Trivial, u32, tx::Nothing> m_valueBufferSize;
	State_impl* m_state;

	[[no_unique_address]] mutable FuncHash m_hash;
	[[no_unique_address]] mutable FuncEqual m_equal;

private:
	// ################ Memory Management ################

	template <class Self>
	decltype(auto) dataAt_impl(this Self&& self, u32 index) {
		tx::const_propagate<Self, u8>* ptr = self.m_data;
		return *impl::at<Entry_impl>(ptr + index * sizeof(Entry_impl));
	}
	template <class Self>
	decltype(auto) valueAt_impl(this Self&& self, u32 index)
	    requires(!Trivial)
	{
		tx::const_propagate<Self, T>* ptr = self.m_value;
		return *impl::at<T>(ptr + index * sizeof(T));
	}

	u32 getEntryCapacity_impl() const { return m_dataBufferSize / sizeof(Entry_impl); }

private:
	// ################ Helpers ################

	template <class Self>
	decltype(auto) getValue_impl(this Self&& self, Entry_impl& entry) {
		if constexpr (Trivial) {
			return entry.value;
		} else {
			return self.valueAt_impl(entry.value);
		}
	}
	size_t clamp_impl(size_t hash) const {
		return hash & (getEntryCapacity_impl() - 1);
	}
	static size_t compact_impl(size_t hash) {
		return hash >> (sizeof(size_t) * impl::ByteSize - Entry_impl::HashSize);
	}
	bool slotOccupied_impl(u32 index) const {
		return dataAt_impl(index).dist != (u8)0xFF;
	}
	u32 advanceIndex_impl(u32 index) const {
		return impl::findPowTwoWrappedPhysIndex(
		    index + 1, getEntryCapacity_impl());
	}

	// @param index the entry to be deleted
	void deleteEntry_impl(u32 index) {
		dataAt_impl(index).dist = (u8)0xFF;
		// no need for further clearing becuase it's going to be overwritten
		// later anyways
		if constexpr (!Trivial) {
		}
		m_state->entryCount--;
	}

private:
	// ################ Logical Implementation ################
	// The Robin Hood Algorithm

	// master function of logical portion
	// @param entry only .value field should be set; others will be set here
	u32 insert_impl(Entry_impl& entry) {
		entry.dist = 0;

		size_t hash = m_hash(getValue_impl(entry));
		entry.hash = compact_impl(hash);
		u32 index = clamp_impl(hash);

		while (slotOccupied_impl(index)) {
			collide_impl(index, entry);
			index = advanceIndex_impl(index);
		}
		dataAt_impl(index) = entry;
		/**
		 * Claim: Within a single insert() call, the global maximum distance
		 *        across the table increases by at most 1.
		 * Proof:
		 *   Claim [1]: A direct collision landed on an entry can never change
		 *              the distance of itself.
		 *   Proof [1]: At that collision, the distance of the incoming entry
		 *              is always 0, which is either same or less then self
		 *              entry, thus resulting the eviction of incoming certain,
		 *              and self remained unchanged.
		 *   Claim [2]: A collision moves everything behind it in a continuous
		 *              stride, and advance the end of the stride by one. And
		 *              it will only ever advance the dist of entries behind it.
		 *   Claim [3]: In one stride of dist, no advancement beyond 1 can be
		 *              produced.
		 *   Proof [3]: Claim [3-1]: Since a collision will not touch the entry
		 *              it's on [1], or anything before it [2], only collision
		 *              happened before entry [y] in `[x][y]` can affect [y].
		 *              Lets run an experiment where [x][y] started as [0][0].
		 *              First collision on [x] (because [3-1]) makes the stride
		 *              become `[0][1][1]`. And then another collision on [x]:
		 *              `[0][1][2][2]`. Another: `[0][1][2][3][3]`. Pattern:
		 *              [y] is not changed after it became 1. Try on earlier
		 *              entry [w]: [w][x][y]: `[0][0][1]` -> `[0][1][2][2]`:
		 *              It indeed advanced y to 2, but also x to 1 as well,
		 *              leaving the difference perfectly equal to 1. And the
		 *              original value of where the collision had landed on
		 *              does not matter, because the advancement of dist always
		 *              start at 0.
		 *   Claim [4]: The dist of an entry will only ever be equal or one
		 *              bigger than the dist of the entry before it.
		 *   Proof [4]: [2],[3]
		 *   Claim [5]: A collision advance the range of [index+n, endOfStride]
		 *              by one. n being the dist of the incoming object when it
		 *              finally surpassed the dist at `index+n`.
		 *   Proof [5]: [3],Proof [3],[4]
		 *   Therefore: Since the biggest dist is always at the end of the
		 *              stride [4], an collision in that stride will always
		 *              advance it [2], and only by 1 [5], therefore Claim
		 *              Proved.
		 * 
		 * With that claim, this entry is considered the last entry before dist
		 * overflowing limit, therefore set allowing the one entry be stored
		 * but set distOverflowing to true and block further inserts.
		 */
		if (entry.dist == MaxDistVal) {
			m_state->distOverflowing = true;
		}
		return index;
	}
	// RobinHood: robTheRichAndHelpThePoor_impl
	// @param index the index of the self entry
	// @param incoming the homeless guy
	void collide_impl(u32 index, Entry_impl& incoming) {
		Entry_impl& self = dataAt_impl(index);
		if (self.dist < incoming.dist) {
			std::swap(incoming, self);
		}
		incoming.dist++;
	}

	// @return the index of the entry of the targeting value in m_data;
	//         InvalidU32 if not found
	u32 find_impl(const T& val) const {
		size_t hash = m_hash(val);
		size_t phash = compact_impl(hash);
		u32 index = clamp_impl(hash);
		u32 dist = 0;

		while (slotOccupied_impl(index)) {
			Entry_impl& entry = dataAt_impl(index);

			if (entry.hash == phash && m_equal(val, getValue_impl(entry)))
				return index;

			// RobinHood Abortion Check
			if (checkAbort_impl(dist, entry.dist)) return InvalidU32;

			index = advanceIndex_impl(index);
			dist++;
		}
		return InvalidU32;
	}
	// RobinHood: Lookup Abortion Check
	// @retern true for abort
	bool checkAbort_impl(u32 distLookup, u32 distEntry) const {
		return distEntry < distLookup;
	}

	// @param index the erased
	void erase_impl(u32 index) {
	}
};
} // namespace tx