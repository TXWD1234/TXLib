// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/data_utils.hpp"
#include "impl/numeric_utils.hpp"
#include "impl/freelist.hpp"
#include "tx/basic_types.hpp"
#include "tx/exception.hpp"
#include "tx/type_traits.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <concepts>

namespace tx {
template <
    class T,
    tx::invocable_r<size_t, const T&> FuncHash = std::hash<T>,
    tx::invocable_r<bool, const T&, const T&> FuncEqual = std::equal_to<T>>
class HashSetOverlay {
private:
	// ################ Traits & Policy ################

	static constexpr bool Trivial =
	    std::is_trivially_move_assignable_v<T> &&
	    std::is_trivially_move_constructible_v<T>;

private:
	// ################ Internal Typedefs & Constants ################

	struct Entry_impl {
		static constexpr u32 HashSize = sizeof(u8) * impl::ByteSize * 3;
		u32 hash : HashSize;
		// dist being 0xFF indicates the entry is empty
		u32 dist : sizeof(u8) * impl::ByteSize = (u8)0xFF;
		std::conditional_t<Trivial, T, u32> value;
	};
	struct State_impl {
		u32 entryCount = 0;
		bool distOverflowing = false;
		[[no_unique_address]] std::conditional_t<
		    !Trivial,
		    typename tx::FreelistOverlay<T>::StateStorage,
		    tx::Nothing> valueFreelistState;
		[[no_unique_address]] mutable FuncHash hash;
		[[no_unique_address]] mutable FuncEqual equal;
	};

	static constexpr f32 MaxLoadFactor = 0.8f;
	static constexpr u8 MaxDistVal = (u8)(0xFF - 1);

	template <class U>
	static constexpr bool value_acceptable =
	    tx::invocable_r<FuncHash, size_t, const U&> &&
	    (tx::invocable_r<FuncEqual, bool, const T&, const U&> ||
	     tx::invocable_r<FuncEqual, bool, const U&, const T&>);


public:
	// ################ Object Lifetime ################

	using value_type = T;

	using EntryStorage = impl::Storage<Entry_impl>;
	using StateStorage = impl::Storage<State_impl>;
	// storage object of the value T if it's none trivial
	using ValueStorage = std::conditional_t<Trivial, void, T>;

	HashSetOverlay(
	    EntryStorage* bufferPtr, u32 bufferSize, StateStorage* statePtr,
	    FuncHash funcHash = FuncHash{}, FuncEqual funcEqual = FuncEqual{})
	    requires Trivial
	    : HashSetOverlay<T>(statePtr, bufferPtr, bufferSize) {
		std::construct_at(m_state);
		initFunctor_impl(funcHash, funcEqual);
	}
	HashSetOverlay(
	    std::span<EntryStorage> buffer, StateStorage* statePtr,
	    FuncHash funcHash = FuncHash{}, FuncEqual funcEqual = FuncEqual{})
	    requires Trivial
	    : HashSetOverlay<T>(buffer.data(), buffer.size(), statePtr,
	                        funcHash, funcEqual) {}
	HashSetOverlay()
	    requires Trivial
	    : m_data(nullptr), m_dataBufferSize(0), m_state(nullptr) {}

	HashSetOverlay(
	    EntryStorage* bufferPtr, u32 bufferSize,
	    ValueStorage* valueBufferPtr, u32 valueBufferSize,
	    StateStorage* statePtr,
	    FuncHash funcHash = FuncHash{}, FuncEqual funcEqual = FuncEqual{})
	    requires(!Trivial)
	    : HashSetOverlay<T>(statePtr,
	                        bufferPtr, bufferSize,
	                        valueBufferPtr, valueBufferSize) {
		std::construct_at(m_state);
		initFunctor_impl(funcHash, funcEqual);
		initFreelist_impl();
	}
	HashSetOverlay(
	    std::span<EntryStorage> buffer, std::span<ValueStorage> valueBuffer,
	    StateStorage* statePtr,
	    FuncHash funcHash = FuncHash{}, FuncEqual funcEqual = FuncEqual{})
	    requires(!Trivial)
	    : HashSetOverlay<T>(
	          buffer.data(), buffer.size(),
	          valueBuffer.data(), valueBuffer.size(),
	          statePtr, funcHash, funcEqual) {}
	HashSetOverlay()
	    requires(!Trivial)
	    : m_data(nullptr), m_value(nullptr),
	      m_dataBufferSize(0), m_valueBufferSize(0),
	      m_state(nullptr) {}

	~HashSetOverlay() {}

	HashSetOverlay(const HashSetOverlay&) = default;
	HashSetOverlay& operator=(const HashSetOverlay&) = default;
	HashSetOverlay(HashSetOverlay&& other) = default;
	HashSetOverlay& operator=(HashSetOverlay&& other) = default;

	// Construct a new object, but preserve the state in StateStorage
	// There must be a live internal state object in StateStorage. It should be
	// from another overlay object
	static HashSetOverlay<T> fromExistingState(
	    EntryStorage* bufferPtr, u32 bufferSize, StateStorage* statePtr)
	    requires Trivial
	{
		HashSetOverlay<T> obj(statePtr, bufferPtr, bufferSize);
		obj.m_state = std::launder(obj.m_state);
		return obj;
	}
	static HashSetOverlay<T> fromExistingState(
	    std::span<EntryStorage> buffer, StateStorage* statePtr)
	    requires Trivial
	{
		HashSetOverlay<T> obj(statePtr, buffer.data(), buffer.size());
		obj.m_state = std::launder(obj.m_state);
		return obj;
	}
	static HashSetOverlay<T> fromExistingState(
	    EntryStorage* bufferPtr, u32 bufferSize,
	    ValueStorage* valueBufferPtr, u32 valueBufferSize,
	    StateStorage* statePtr)
	    requires(!Trivial)
	{
		HashSetOverlay<T> obj(statePtr,
		                      bufferPtr, bufferSize,
		                      valueBufferPtr, valueBufferSize);
		obj.m_state = std::launder(obj.m_state);
		obj.initFreelistExisting_impl();
		return obj;
	}
	static HashSetOverlay<T> fromExistingState(
	    std::span<EntryStorage> buffer, std::span<ValueStorage> valueBuffer,
	    StateStorage* statePtr)
	    requires(!Trivial)
	{
		return fromExistingState(
		    buffer.data(), buffer.size(),
		    valueBuffer.data(), valueBuffer.size(),
		    statePtr);
	}

	bool valid() const
	    requires Trivial
	{ return m_state && m_data && m_dataBufferSize; }
	bool valid() const
	    requires(!Trivial)
	{ return m_state && m_data && m_dataBufferSize && m_value && m_valueBufferSize; }

	void destruct() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		if constexpr (!Trivial) {
			m_valueFreelist.destruct();
		}
		std::destroy_at(m_state);
	}

public:
	// ################ Public Interface ################

	// @return the handle of the inserted entry. It will be invalidated after
	//         any modification
	template <class V>
	    requires std::constructible_from<T, V&&>
	u32 insert(V&& val) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(
		    [this] { return !this->full(); },
		    [this] {
			    return std::format(
			        "Bad expension. {}.",
			        this->m_state->distOverflowing ?
			            "RobinHood: Entry distance overflowing." :
			            std::format("Size overflows buffer capacity."
			                        " size = {}; capacity = {}; expansionCount = 1",
			                        this->size(), this->capacity()));
		    });
		Entry_impl entry;
		if constexpr (Trivial) {
			entry.value = std::forward<V>(val);
		} else {
			// entry.value is the index in value buffer of this entry
			entry.value = m_valueFreelist.allocate();
			std::construct_at(
			    m_value + entry.value,
			    std::forward<V>(val));
		}
		m_state->entryCount++;
		return insert_impl(entry);
	}
	// @return the index of the erased object
	u32 erase(u32 index) {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		impl::assert_impl(impl::assert::out_of_range(getEntryCapacity_impl(), index));
		impl::assert_impl(
		    [this, index] { return this->slotOccupied_impl(index); },
		    [index] { return std::format(
			              "Bad access. Entry does not exist. index = {}", index); });
		deleteEntry_impl(index);
		dataAt_impl(erase_impl(index)).dist = (u8)0xFF;
		return index;
	}

	template <class U>
	    requires value_acceptable<U>
	u32 find(const U& val) const {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		return find_impl(val);
	}
	template <class U>
	    requires value_acceptable<U>
	bool exist(const U& val) const {
		return find(val) != InvalidU32;
	}

	void clear() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		if constexpr (!Trivial) {
			for (u32 i = 0; i < getEntryCapacity_impl(); i++) {
				if (slotOccupied_impl(i)) std::destroy_at(m_value + dataAt_impl(i).value);
			}
			m_valueFreelist.clear();
		}
		for (u32 i = 0; i < getEntryCapacity_impl(); i++) {
			dataAt_impl(i).dist = (u8)0xFF;
		}
		m_state->entryCount = 0;
		m_state->distOverflowing = false;
	}

	u32 size() {
		impl::assert_impl(impl::assert::overlay_object_valid(this));
		return m_state->entryCount;
	}
	u32 capacity() { return getEntryCapacity_impl() * MaxLoadFactor; }
	bool empty() { return !size(); }
	bool full() { return size() >= capacity() || m_state->distOverflowing; }

	template <class Self>
	tx::const_propagate<Self, T>& at(this Self&& self, u32 index) {
		impl::assert_impl(impl::assert::overlay_object_valid(&self));
		impl::assert_impl(impl::assert::out_of_range(self.getEntryCapacity_impl(), index));
		return self.getValue_impl(self.dataAt_impl(index));
	}

private:
	// ################ Runtime Data ################

	u8* m_data;
	[[no_unique_address]] std::conditional_t<
	    !Trivial, T*, tx::Nothing> m_value;
	u32 m_dataBufferSize;
	[[no_unique_address]] std::conditional_t<
	    !Trivial, u32, tx::Nothing> m_valueBufferSize;
	State_impl* m_state;
	[[no_unique_address]] std::conditional_t<
	    !Trivial, tx::FreelistOverlay<T>, tx::Nothing> m_valueFreelist;

private:
	// ################ Construction Helpers ################

	HashSetOverlay(
	    StateStorage* statePtr,
	    EntryStorage* bufferPtr, u32 bufferSize)
	    requires Trivial
	    : m_data(tx::isPowTwo(bufferSize) ? reinterpret_cast<u8*>(bufferPtr) : nullptr),
	      m_dataBufferSize(tx::isPowTwo(bufferSize) ? bufferSize * sizeof(Entry_impl) : 0),
	      m_state(reinterpret_cast<State_impl*>(statePtr)) {
		impl::assert_impl(
		    [&] { return valid(); },
		    [&] {
			    if (!tx::isPowTwo(bufferSize)) {
				    return std::format(
				        "Bad construction. Argument `bufferSize` have to be power of 2. bufferSize = {}",
				        bufferSize);
			    } else {
				    return std::format(
				        "Bad construction. Invalid pointers provided. bufferPtr = {}; statePtr = {}",
				        static_cast<const void*>(bufferPtr),
				        static_cast<const void*>(statePtr));
			    }
		    });
		std::uninitialized_value_construct(
		    reinterpret_cast<Entry_impl*>(bufferPtr),
		    reinterpret_cast<Entry_impl*>(bufferPtr + bufferSize));
	}
	HashSetOverlay(
	    StateStorage* statePtr,
	    EntryStorage* bufferPtr, u32 bufferSize,
	    ValueStorage* valueBufferPtr, u32 valueBufferSize)
	    requires(!Trivial)
	    : m_data(tx::isPowTwo(bufferSize) ? reinterpret_cast<u8*>(bufferPtr) : nullptr),
	      m_value(tx::isPowTwo(bufferSize) && (valueBufferSize == bufferSize) ? valueBufferPtr : nullptr),
	      m_dataBufferSize(tx::isPowTwo(bufferSize) ? bufferSize * sizeof(Entry_impl) : 0),
	      m_valueBufferSize(tx::isPowTwo(bufferSize) && (valueBufferSize == bufferSize) ? valueBufferSize * sizeof(T) : 0),
	      m_state(reinterpret_cast<State_impl*>(statePtr)) {
		impl::assert_impl(
		    [&] { return valid(); },
		    [&] {
			    if (!tx::isPowTwo(bufferSize)) {
				    return std::format(
				        "Bad construction. Argument `bufferSize` have to be power of 2. bufferSize = {}",
				        bufferSize);
			    } else if (valueBufferSize != bufferSize) {
				    return std::format(
				        "Bad construction. Argument `valueBufferSize` must equal `bufferSize`. valueBufferSize = {}, bufferSize = {}",
				        valueBufferSize, bufferSize);
			    } else {
				    return std::format(
				        "Bad construction. Invalid pointers provided. bufferPtr = {}; valueBufferPtr = {}; statePtr = {}",
				        static_cast<const void*>(bufferPtr),
				        static_cast<const void*>(valueBufferPtr),
				        static_cast<const void*>(statePtr));
			    }
		    });
		std::uninitialized_value_construct(
		    reinterpret_cast<Entry_impl*>(bufferPtr),
		    reinterpret_cast<Entry_impl*>(bufferPtr + bufferSize));
	}

	void initFreelist_impl()
	    requires(!Trivial)
	{
		m_valueFreelist = tx::FreelistOverlay<T>(
		    m_value, m_valueBufferSize, &m_state->valueFreelistState);
	}
	void initFreelistExisting_impl()
	    requires(!Trivial)
	{
		m_valueFreelist = tx::FreelistOverlay<T>::fromExistingState(
		    m_value, m_valueBufferSize, &m_state->valueFreelistState);
	}

	void initFunctor_impl(FuncHash funcHash, FuncEqual funcEqual) {
		m_state->hash = funcHash;
		m_state->equal = funcEqual;
	}

private:
	// ################ Memory Management ################

	template <class Self>
	tx::const_propagate<Self, Entry_impl>& dataAt_impl(this Self&& self, u32 index) {
		tx::const_propagate<Self, u8>* ptr = self.m_data;
		return *impl::at<Entry_impl>(ptr + index * sizeof(Entry_impl));
	}
	template <class Self>
	tx::const_propagate<Self, T>& valueAt_impl(this Self&& self, u32 index)
	    requires(!Trivial)
	{
		tx::const_propagate<Self, T>* ptr = self.m_value;
		return *(ptr + index);
	}

	u32 getEntryCapacity_impl() const {
		return m_dataBufferSize / sizeof(Entry_impl);
	}

private:
	// ################ Helpers ################

	template <class Self>
	tx::const_propagate<Self, T>& getValue_impl(
	    this Self&& self,
	    tx::const_propagate<Self, Entry_impl>& entry) {
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
	u32 wrapIndex_impl(u32 index) const {
		return impl::findPowTwoWrappedPhysIndex(
		    index, getEntryCapacity_impl());
	}

	// @param index the entry to be deleted
	void deleteEntry_impl(u32 index) {
		// didn't set dist to 0xFF here because it belongs to logic, not state
		// managing. Also no changes to the rest of the Entry_impl because all
		// those are going to be overwritten later anyways
		if constexpr (!Trivial) {
			// clean up for non trivial T
			u32 valueIndex = dataAt_impl(index).value;
			std::destroy_at(m_value + valueIndex);
			m_valueFreelist.free(valueIndex);
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

		size_t hash = m_state->hash(getValue_impl(entry));
		entry.hash = compact_impl(hash);
		u32 index = clamp_impl(hash);

		while (slotOccupied_impl(index)) {
			collide_impl(index, entry);
			index = wrapIndex_impl(index + 1);
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
	template <class U>
	    requires value_acceptable<U>
	u32 find_impl(const U& val) const {
		size_t hash = m_state->hash(val);
		size_t phash = compact_impl(hash);
		u32 index = clamp_impl(hash);
		u32 dist = 0;

		while (slotOccupied_impl(index)) {
			const Entry_impl& entry = dataAt_impl(index);

			if constexpr (tx::invocable_r<FuncEqual, bool, const T&, const U&>) {
				if (entry.hash == phash && m_state->equal(getValue_impl(entry), val)) return index;
			} else {
				if (entry.hash == phash && m_state->equal(val, getValue_impl(entry))) return index;
			}

			// RobinHood Abortion Check
			if (checkAbort_impl(dist, entry.dist)) return InvalidU32;

			index = wrapIndex_impl(index + 1);
			dist++;
		}
		return InvalidU32;
	}
	// RobinHood: Lookup Abortion Check
	// @retern true for abort
	bool checkAbort_impl(u32 distLookup, u32 distEntry) const {
		return distEntry < distLookup;
	}

	// RobinHood: Post deletion move
	// @param index the erased
	// @return the final entry to be set to 0xFF
	u32 erase_impl(u32 index) {
		// Phase 1: find the end
		u32 end = wrapIndex_impl(index + 1);
		while (slotOccupied_impl(end) &&
		       dataAt_impl(end).dist) { end = wrapIndex_impl(end + 1); }

		// Phase 2: move
		for (u32 i = wrapIndex_impl(index + 1);
		     i != end;
		     i = wrapIndex_impl(i + 1)) {
			dataAt_impl(i).dist--;
			dataAt_impl(wrapIndex_impl(i - 1)) = dataAt_impl(i);
		}
		return end;
	}
};
} // namespace tx