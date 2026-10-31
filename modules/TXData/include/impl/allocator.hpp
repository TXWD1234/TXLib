// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/data_foundation.hpp"
#include "impl/numeric_utils.hpp"
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include <type_traits>

namespace tx {
// ################ Allocator Associated Concepts ################

// allocator
template <class Alloc>
struct is_allocator {
	static constexpr bool value = requires(
	    Alloc a, std::size_t n, typename Alloc::value_type* ptr) {
		typename Alloc::value_type;
		{ a.allocate(n) } -> std::same_as<typename Alloc::value_type*>;
		{ a.deallocate(ptr, n) } -> std::same_as<void>;
	};
};
template <class Alloc>
constexpr bool is_allocator_v = is_allocator<Alloc>::value;
template <class Alloc>
concept allocator = is_allocator_v<Alloc>;

// instantiated allocator_trait
// The value type of the Traits is not checked, only the validity of it being
// an allocator traits
template <class Traits>
concept allocator_trait =
    requires {
	typename Traits::allocator_type;
	typename Traits::value_type;
	typename Traits::pointer;

	// using std::uint32_t here as dummy rebinding type
	typename Traits::template rebind_alloc<std::uint32_t>;
	typename Traits::template rebind_traits<std::uint32_t>; } &&
    requires(
        typename Traits::allocator_type& alloc,
        typename Traits::size_type n,
        typename Traits::pointer ptr) {
	    { Traits::allocate(alloc, n) } -> tx::satisfies_trait<std::is_pointer>;
	    Traits::deallocate(alloc, ptr, n);
    };

// ################ Implementation Utilities ################
namespace impl {
template <tx::allocator Allocator, class T>
using rebind_alloc = typename std::allocator_traits<Allocator>::
    template rebind_alloc<T>;
}

// ################ Custom Allocator Traits ################

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
	[[nodiscard]] static Byte* allocate(const Allocator& alloc, size_type bytes) {
		alloc_t aligned_alloc(alloc);
		return reinterpret_cast<Byte*>(traits::allocate(
		    aligned_alloc, tx::divCeil(bytes, Alignment)));
	}

	static void deallocate(const Allocator& alloc, Byte* ptr, size_type bytes) noexcept {
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
	[[nodiscard]] static pointer allocate(const Allocator& alloc, size_type size) {
		alloc_t typed_alloc(alloc);
		return base_traits::allocate(typed_alloc, size);
	}

	static void deallocate(const Allocator& alloc, pointer ptr, size_type size) noexcept {
		alloc_t typed_alloc(alloc);
		base_traits::deallocate(typed_alloc, ptr, size);
	}
};

namespace impl {
namespace details {
using allocator_last_arg_pred =
    decltype([]<class U>(std::type_identity<U>) {
	    return tx::allocator<U>;
    });
} // namespace details

template <class... Args>
inline constexpr bool VerifyLastAllocArg = impl::verify_last_arg_v<
    details::allocator_last_arg_pred, Args...>;

// alloc_t
template <class Fallback, class... Args>
using ParseLastAllocArg = impl::parse_last_arg_t<
    Fallback, details::allocator_last_arg_pred, Args...>;

// tx::type_list_t<remaining_args...>
template <class... Args>
using ForwardLastAllocArgRemainder =
    impl::foward_last_arg_remainder_t<
        details::allocator_last_arg_pred, Args...>;
} // namespace impl
} // namespace tx