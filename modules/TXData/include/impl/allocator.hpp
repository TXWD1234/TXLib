// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/data_foundation.hpp"
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include "impl/numeric_utils.hpp"

namespace tx {
namespace impl {
template <tx::allocator Allocator, class T>
using rebind_alloc = typename std::allocator_traits<Allocator>::
    template rebind_alloc<T>;
}

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
} // namespace tx