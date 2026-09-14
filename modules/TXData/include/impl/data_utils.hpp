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
// ################ Storage ################

template <class T>
struct alignas(T) Storage {
	std::byte data[sizeof(T)];
};
template <std::size_t Size>
struct alignas(Size) StorageSized {
	std::byte data[Size];
};

// ################ Allocator ################

template <tx::allocator Allocator, class T>
using rebind_alloc = typename std::allocator_traits<Allocator>::
    template rebind_alloc<T>;
} // namespace impl

// @param Allocator can just be an arbitrary allocator instance. The type of it does
// not matter since it's going to be rebinded anyways.
// @param Alignment must be a power of 2
template <class Allocator, size_t Alignment, tx::byte_like Byte = u8>
struct aligned_allocator_traits : std::allocator_traits<Allocator> {
	static_assert(tx::isPowTwo(Alignment),
	              "tx::aligned_allocator_traits: Bad instantiation. "
	              "Template parameter `Alignment` must be a power of 2.");

private:
	using storage_t = impl::StorageSized<Alignment>;
	using alloc_t = impl::rebind_alloc<Allocator, storage_t>;
	using traits = std::allocator_traits<alloc_t>;

public:
	// this does virtually nothing, exists only to match the standard
	template <typename U>
	using rebind_traits = aligned_allocator_traits<
	    impl::rebind_alloc<Allocator, U>,
	    Alignment, Byte>;

	using value_type = Byte;
	using pointer = Byte*;
	using size_type = typename traits::size_type;

public:
	[[nodiscard]] static Byte* allocate(Allocator& alloc, size_type bytes) {
		alloc_t aligned_alloc(alloc);
		return reinterpret_cast<Byte*>(traits::allocate(
		    aligned_alloc, tx::divCeil(bytes, Alignment)));
	}

	static void deallocate(Allocator& alloc, Byte* ptr, size_type bytes) noexcept {
		alloc_t aligned_alloc(alloc);
		traits::deallocate(
		    aligned_alloc,
		    reinterpret_cast<storage_t*>(ptr),
		    tx::divCeil(bytes, Alignment));
	}
};

template <class Allocator, class T>
struct typed_allocator_traits
    : std::allocator_traits<impl::rebind_alloc<Allocator, T>> {
private:
	using base_traits = std::allocator_traits<impl::rebind_alloc<Allocator, T>>;
	using alloc_t = typename base_traits::allocator_type;

public:
	template <typename U>
	using rebind_traits = typed_allocator_traits<
	    impl::rebind_alloc<Allocator, U>, U>;

	using pointer = typename base_traits::pointer;
	using size_type = typename base_traits::size_type;
	using allocator_type = Allocator;

public:
	// passing the original Allocator directly is allowed
	[[nodiscard]] static pointer allocate(Allocator& alloc, size_type size) {
		alloc_t typed_alloc(alloc);
		return base_traits::allocate(typed_alloc, size);
	}

	static void deallocate(Allocator& alloc, pointer ptr, size_type size) noexcept {
		alloc_t typed_alloc(alloc);
		base_traits::deallocate(typed_alloc, ptr, size);
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