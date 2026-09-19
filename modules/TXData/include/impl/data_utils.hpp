// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/numeric_utils.hpp"
#include "impl/allocator.hpp"
#include "tx/basic_types.hpp"
#include "tx/exception.hpp"
#include "tx/type_traits.hpp"
#include <concepts>
#include <memory>
#include <cstring>
#include <numeric>

namespace tx {
// ################ General Utilities ################

template <class T,
          tx::invocable_r<T*, u32> AllocFunc,
          std::invocable<T*, u32> DeallocFunc>
inline T* resize(
    T*& data, u32 currentSize,
    u32 targetCapacity, u32 currentCapacity,
    AllocFunc&& allocFunc, DeallocFunc&& deallocFunc) {
	if (targetCapacity < currentSize) return data;

	T* newData = allocFunc(targetCapacity);

	if (data) {
		std::uninitialized_move(data, data + currentSize, newData);
		std::destroy(data, data + currentSize);
		deallocFunc(data, currentCapacity);
	}

	data = newData;
	return data;
}

template <class T,
          tx::allocator Allocator = std::allocator<T>,
          class AllocatorTrait = std::allocator_traits<Allocator>>
    requires tx::allocator_trait<AllocatorTrait>
inline T* resize(
    T*& data, u32 currentSize,
    u32 targetSize, u32 currentCapacity,
    Allocator alloc = Allocator()) {
	using alloc_t = typename AllocatorTrait::template rebind_alloc<T>;
	using alloc_traits = typename AllocatorTrait::template rebind_traits<T>;
	alloc_t bound_alloc = alloc_t(alloc);

	return resize(
	    data, currentSize, targetSize, currentCapacity,
	    [&](u32 size) { return alloc_traits::allocate(bound_alloc, size); },
	    [&](T* ptr, u32 size) { alloc_traits::deallocate(bound_alloc, ptr, size); });
}

namespace impl {
// DevNote: move to data_utils.hpp
template <class T, std::invocable<T, T> CompareFunc>
inline bool isSame(const T& a, const T& b, CompareFunc&& cmp) {
	return !cmp(a, b) && !cmp(b, a);
}
} // namespace impl

// buffer must be sorted
// not found returns 0xFFFFFFFF
template <class T, tx::invocable_r<bool, T, T> CompareFunc>
inline u32 binarySearch(
    const T& val,
    T* bufferPtr, u32 bufferSize,
    CompareFunc&& cmp = CompareFunc{}) {
	auto it = std::lower_bound(bufferPtr, bufferPtr + bufferSize, val, cmp);
	if (it != bufferPtr + bufferSize && impl::isSame(*it, val, cmp))
		return findIteratorIndex(bufferPtr, it);
	return InvalidU32;
}

constexpr size_t CacheLineSize = 64;

template <class T>
struct alignas(CacheLineSize) alignas(T) CacheLineStorage {
	static constexpr size_t ElementCount =
	    std::lcm(sizeof(T), CacheLineSize) / sizeof(T);
	std::byte data[ElementCount * sizeof(T)];
};

template <std::input_iterator It>
inline u32 findIteratorIndex(It begin, It it) {
	return static_cast<u32>(std::distance(begin, it));
}

template <class T>
constexpr inline u8* nextAlign(u8* ptr) {
	return reinterpret_cast<u8*>(
	    (reinterpret_cast<uintptr_t>(ptr) + alignof(T) - 1) &
	    ~(uintptr_t)(alignof(T) - 1));
}
template <class T>
constexpr inline u32 nextAlign(u32 index) {
	return (index + alignof(T) - 1) & ~(u32)(alignof(T) - 1);
}

struct IndexRange {
	u32 offset = 0;
	u32 size = 0;

	[[nodiscard]] constexpr u32 begin() const noexcept { return offset; }
	[[nodiscard]] constexpr u32 end() const noexcept { return offset + size; }
	[[nodiscard]] constexpr bool empty() const noexcept { return size == 0; }
};

// ################ STL Addon ################

// Wrapper function that groups std::uninitialized_move and std::destroy
// @return the last iterator of the destination
template <typename InputIt, typename ForwardIt>
ForwardIt uninitialized_relocate(InputIt first, InputIt last, ForwardIt destFirst) {
	ForwardIt destLast = std::uninitialized_move(first, last, destFirst);
	std::destroy(first, last);
	return destLast;
}

// ################ TXData Implementation Utilities ################
namespace impl {
// std::launder(reinterpret_cast<T*>(ptr))
template <class T>
inline T* at(u8* ptr) {
	return std::launder(reinterpret_cast<T*>(ptr));
}
// std::launder(reinterpret_cast<const T*>(ptr))
template <class T>
inline const T* at(const u8* ptr) {
	return std::launder(reinterpret_cast<const T*>(ptr));
}
// std::bit_cast but for raw pointers
// memcpy(&ret, ptr, sizeof(T))
template <class T>
inline T as(const u8* ptr) {
	T ret;
	std::memcpy(&ret, ptr, sizeof(T));
	return ret;
}

constexpr u32 ByteSize = 8;

// @param size have to be power of 2
template <std::integral T>
inline constexpr T findPowTwoWrappedPhysIndex(T index, T size) {
	impl::assert_impl(
	    [&] { return tx::isPowTwo(size); },
	    [=] { return std::format(
		          "Invalid parameter. Size must be power of 2. "
		          "size = {}",
		          size); });
	return index & (size - (T)1);
}
} // namespace impl
} // namespace tx