// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/allocator.hpp"
#include "tx/basic_types.hpp"

namespace tx {
/**
 * @brief A basic RAII wrapper for heap memory
 */
template <class T, tx::allocator Allocator>
struct BasicStorage {
	using traits = tx::typed_allocator_traits<Allocator, T>;
	BasicStorage(u32 size, Allocator alloc)
	    : data(traits::allocate(alloc, size)),
	      size(size), allocator(alloc) {}
	BasicStorage() {}
	~BasicStorage() { destruct_impl(); }

	BasicStorage(const BasicStorage&) = delete;
	BasicStorage& operator=(const BasicStorage&) = delete;
	BasicStorage(BasicStorage&& other)
	    : data(other.data), size(other.size), allocator(other.allocator) {
		other.null_impl();
	}
	BasicStorage& operator=(BasicStorage&& other) {
		destruct_impl();
		data = other.data;
		size = other.size;
		allocator = other.allocator;
		other.null_impl();
		return *this;
	}

	T* data = nullptr;
	u32 size = 0;
	[[no_unique_address]] Allocator allocator;

private:
	void null_impl() {
		data = nullptr;
		size = 0;
	}
	void destruct_impl() {
		if (data) traits::deallocate(allocator, data, size);
	}
};
/**
 * POCMA stubbed and potentially will not be implemented.
 */
} // namespace tx