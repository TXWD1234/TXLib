// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include <memory>

namespace tx::impl {

// ################ Storage ################

template <class T>
struct alignas(T) Storage {
	std::byte data[sizeof(T)];
};
template <std::size_t Size>
struct alignas(Size) StorageSized {
	std::byte data[Size];
};
template <class T>
struct StorageHolder {
	T storage;
};

} // namespace tx::impl