// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/basic_utils.hpp"

namespace tx {
template <std::input_iterator It>
inline u32 findIteratorIndex(It begin, It it) {
	return static_cast<u32>(std::distance(begin, it)); // O(n)
}
} // namespace tx