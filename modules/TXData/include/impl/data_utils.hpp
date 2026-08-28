// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/numeric_utils.hpp"
#include "tx/basic_types.hpp"
#include "tx/exception.hpp"
#include "tx/type_traits.hpp"
#include <memory>

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

template <class T>
inline T* allocate(u32 size) {
	if (size == 0) return nullptr;
	return static_cast<T*>(::operator new(
	    size * sizeof(T), std::align_val_t{ alignof(T) }));
}
template <class T>
inline void free(T* ptr) {
	if (!ptr) return;
	::operator delete(ptr, std::align_val_t{ alignof(T) });
}

template <class T, tx::allocator Allocator = std::allocator<T>>
inline void resize(
    T*& data, u32 currentSize,
    u32 targetSize, u32 currentCapacity = InvalidU32,
    const Allocator& alloc = Allocator()) {
	using alloc_t = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
	using alloc_traits = std::allocator_traits<alloc_t>;

	if (currentCapacity == InvalidU32) currentCapacity = currentSize;
	if (targetSize < currentSize) return;

	alloc_t allocator(alloc);
	T* newData = alloc_traits::allocate(allocator, targetSize);

	if (data) {
		std::uninitialized_move(data, data + currentSize, newData);
		std::destroy(data, data + currentSize);
		alloc_traits::deallocate(allocator, data, currentCapacity);
	}

	data = newData;
}

struct IndexRange {
	u32 offset = 0;
	u32 size = 0;

	[[nodiscard]] constexpr u32 begin() const noexcept { return offset; }
	[[nodiscard]] constexpr u32 end() const noexcept { return offset + size; }
	[[nodiscard]] constexpr bool empty() const noexcept { return size == 0; }
};

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
	impl::assert_impl([&]() { return tx::isPowTwo(size); },
	                  "tx::impl::findPowTwoWrappedPhysIndex(): Invalid parameter value: `size` is not power of 2");
	return index & (size - (T)1);
}

template <class T>
struct alignas(alignof(T)) Storage {
	std::byte data[sizeof(T)];
};
} // namespace impl
} // namespace tx