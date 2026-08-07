// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include "tx/exception.hpp"

namespace tx {
template <std::input_iterator It>
inline u32 findIteratorIndex(It begin, It it) {
	return static_cast<u32>(std::distance(begin, it));
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

struct IndexRange {
	u32 offset = 0;
	u32 size = 0;

	[[nodiscard]] constexpr u32 begin() const noexcept { return offset; }
	[[nodiscard]] constexpr u32 end() const noexcept { return offset + size; }
	[[nodiscard]] constexpr bool empty() const noexcept { return size == 0; }
};

// ============================================
// **************** Std Addons ****************
// ============================================

template <typename Alloc, typename FwdIt>
FwdIt uninitialized_default_construct_a(Alloc& alloc, FwdIt first, FwdIt last) {
	using AT = std::allocator_traits<Alloc>;
	FwdIt cur = first;
	try {
		for (; cur != last; ++cur)
			AT::construct(alloc, std::addressof(*cur));
		return cur;
	} catch (...) {
		for (; first != cur; ++first)
			AT::destroy(alloc, std::addressof(*first));
		throw;
	}
}
template <typename Alloc, typename FwdIt>
FwdIt uninitialized_value_construct_a(Alloc& alloc, FwdIt first, FwdIt last) {
	using AT = std::allocator_traits<Alloc>;
	using T = typename std::iterator_traits<FwdIt>::value_type;
	FwdIt cur = first;
	try {
		for (; cur != last; ++cur)
			AT::construct(alloc, std::addressof(*cur), T());
		return cur;
	} catch (...) {
		for (; first != cur; ++first)
			AT::destroy(alloc, std::addressof(*first));
		throw;
	}
}
template <typename Alloc, typename InputIt, typename FwdIt>
FwdIt uninitialized_move_a(Alloc& alloc, InputIt first, InputIt last, FwdIt dest) {
	using AT = std::allocator_traits<Alloc>;
	FwdIt cur = dest;
	try {
		for (; first != last; ++first, ++cur)
			AT::construct(alloc, std::addressof(*cur), std::move(*first));
		return cur;
	} catch (...) {
		for (; dest != cur; ++dest)
			AT::destroy(alloc, std::addressof(*dest));
		throw;
	}
}
template <typename Alloc, typename InputIt, typename Size, typename FwdIt>
FwdIt uninitialized_move_n_a(Alloc& alloc, InputIt first, Size n, FwdIt dest) {
	using AT = std::allocator_traits<Alloc>;
	FwdIt cur = dest;
	try {
		for (; n > 0; --n, ++first, ++cur)
			AT::construct(alloc, std::addressof(*cur), std::move(*first));
		return cur;
	} catch (...) {
		for (; dest != cur; ++dest)
			AT::destroy(alloc, std::addressof(*dest));
		throw;
	}
}
template <typename Alloc, typename InputIt, typename FwdIt>
FwdIt uninitialized_copy_a(Alloc& alloc, InputIt first, InputIt last, FwdIt dest) {
	using AT = std::allocator_traits<Alloc>;
	FwdIt cur = dest;
	try {
		for (; first != last; ++first, ++cur)
			AT::construct(alloc, std::addressof(*cur), *first);
		return cur;
	} catch (...) {
		for (; dest != cur; ++dest)
			AT::destroy(alloc, std::addressof(*dest));
		throw;
	}
}
template <typename Alloc, typename InputIt, typename Size, typename FwdIt>
FwdIt uninitialized_copy_n_a(Alloc& alloc, InputIt first, Size n, FwdIt dest) {
	using AT = std::allocator_traits<Alloc>;
	FwdIt cur = dest;
	try {
		for (; n > 0; --n, ++first, ++cur)
			AT::construct(alloc, std::addressof(*cur), *first);
		return cur;
	} catch (...) {
		for (; dest != cur; ++dest)
			AT::destroy(alloc, std::addressof(*dest));
		throw;
	}
}
template <typename Alloc, typename FwdIt, typename T>
void uninitialized_fill_a(Alloc& alloc, FwdIt first, FwdIt last, const T& value) {
	using AT = std::allocator_traits<Alloc>;
	FwdIt cur = first;
	try {
		for (; cur != last; ++cur)
			AT::construct(alloc, std::addressof(*cur), value);
	} catch (...) {
		for (; first != cur; ++first)
			AT::destroy(alloc, std::addressof(*first));
		throw;
	}
}
template <typename Alloc, typename FwdIt, typename Size, typename T>
FwdIt uninitialized_fill_n_a(Alloc& alloc, FwdIt first, Size n, const T& value) {
	using AT = std::allocator_traits<Alloc>;
	FwdIt cur = first;
	try {
		for (; n > 0; --n, ++cur)
			AT::construct(alloc, std::addressof(*cur), value);
		return cur;
	} catch (...) {
		for (; first != cur; ++first)
			AT::destroy(alloc, std::addressof(*first));
		throw;
	}
}
} // namespace tx