// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/numeric_utils.hpp"
#include "tx/basic_types.hpp"
#include "tx/exception.hpp"
#include "tx/type_traits.hpp"
#include <memory>
#include <cstring>
#include <numeric>

namespace tx {
template <std::input_iterator It>
inline u32 findIteratorIndex(It begin, It it) {
	return static_cast<u32>(std::distance(begin, it));
}

template <class T>
constexpr inline u8* nextAlign(u8* ptr) {
	return reinterpret_cast<u8*>((reinterpret_cast<uintptr_t>(ptr) + alignof(T) - 1) & ~(uintptr_t)(alignof(T) - 1));
}
template <class T>
constexpr inline u32 nextAlign(u32 index) {
	return (index + alignof(T) - 1) & ~(u32)(alignof(T) - 1);
}

namespace impl {
template <class T>
struct alignas(T) Storage {
	std::byte data[sizeof(T)];
};
template <std::size_t Size>
struct alignas(Size) StorageSized {
	std::byte data[Size];
};
} // namespace impl

template <class Allocator, size_t Alignment>
struct aligned_allocator_traits : std::allocator_traits<Allocator> {
	static_assert(tx::isPowTwo(Alignment),
	              "tx::aligned_allocator_traits: Bad instantiation. "
	              "Template parameter `Alignment` must be a power of 2.");

private:
	using storage_t = impl::StorageSized<Alignment>;
	using alloc_t = typename std::allocator_traits<Allocator>::
	    template rebind_alloc<storage_t>;
	using traits = std::allocator_traits<alloc_t>;

public:
	template <typename U>
	using rebind_traits = aligned_allocator_traits<
	    typename std::allocator_traits<Allocator>::template rebind_alloc<U>,
	    Alignment>;

public:
	template <tx::byte_like T = u8>
	[[nodiscard]] static T* allocate(Allocator& alloc, std::size_t bytes) {
		alloc_t aligned_alloc(alloc);
		return reinterpret_cast<T*>(traits::allocate(
		    aligned_alloc, tx::divCeil(bytes, Alignment)));
	}

	template <tx::byte_like T = u8>
	static void deallocate(Allocator& alloc, T* ptr, std::size_t bytes) noexcept {
		alloc_t aligned_alloc(alloc);
		traits::deallocate(
		    aligned_alloc,
		    reinterpret_cast<storage_t*>(ptr),
		    tx::divCeil(bytes, Alignment));
	}
};

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

struct IndexRange {
	u32 offset = 0;
	u32 size = 0;

	[[nodiscard]] constexpr u32 begin() const noexcept { return offset; }
	[[nodiscard]] constexpr u32 end() const noexcept { return offset + size; }
	[[nodiscard]] constexpr bool empty() const noexcept { return size == 0; }
};

constexpr size_t CacheLineSize = 64;

template <class T>
struct alignas(CacheLineSize) alignas(T) CacheLineStorage {
	static constexpr size_t ElementCount =
	    std::lcm(sizeof(T), CacheLineSize) / sizeof(T);
	std::byte data[ElementCount * sizeof(T)];
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

namespace impl {
template <class T>
inline T* at(u8* ptr) {
	return std::launder(reinterpret_cast<T*>(ptr));
}
template <class T>
inline const T* at(const u8* ptr) {
	return std::launder(reinterpret_cast<const T*>(ptr));
}

constexpr u32 ByteSize = 8;

// @param size have to be power of 2
template <std::integral T>
inline constexpr T findPowTwoWrappedPhysIndex(T index, T size) {
	impl::assert_impl([&] { return tx::isPowTwo(size); },
	                  "Invalid parameter value: size is not power of 2.");
	return index & (size - (T)1);
}
} // namespace impl
} // namespace tx