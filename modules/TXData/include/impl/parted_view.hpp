// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "tx/basic_types.hpp"
#include <span>

namespace tx {
template <class T>
class UniformPartedView {
public:
	UniformPartedView() = default;
	UniformPartedView(T* data, u32 partSize) : m_data(data), m_partSize(partSize) {}

	UniformPartedView(const UniformPartedView& other) = default;
	UniformPartedView& operator=(const UniformPartedView& other) = default;
	UniformPartedView(UniformPartedView&& other) = default;
	UniformPartedView& operator=(UniformPartedView&& other) = default;

public:
	using partition = std::span<T>;

	partition at(u32 partIndex) const {
		return at(partIndex, m_data, m_partSize);
	}
	partition operator[](u32 partIndex) const { return at(partIndex); }

	static partition at(u32 partIndex, T* data, u32 partSize) {
		return std::span<T>(data + partIndex * partSize,
		                    data + partIndex * partSize + partSize);
	}

private:
	T* m_data = nullptr;
	u32 m_partSize = 0;
};
} // namespace tx