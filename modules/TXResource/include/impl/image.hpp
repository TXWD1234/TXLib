// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXResource

#pragma once
#include "stb_image.hpp"
#include "tx/math.h"
#include <span>
#include "impl/io_file.hpp"

namespace tx {

class Image {
public:
	Image() = default;
	Image(const fs::path& filePath)
	    : m_loaded(true) {
		m_data = stbi_load(filePath.string().c_str(), &m_dimension.x, &m_dimension.y, &m_channelCount, 4);
		m_valid = m_data;
	}
	~Image() {
		if (m_data)
			stbi_image_free(m_data);
	}

	Image(const Image&) = delete;
	Image& operator=(const Image&) = delete;
	Image(Image&& other) noexcept
	    : m_data(other.m_data),
	      m_dimension(other.m_dimension),
	      m_channelCount(other.m_channelCount),
	      m_loaded(other.m_loaded),
	      m_valid(other.m_valid) {
		other.m_data = nullptr;
		other.m_valid = false;
	}

	Image& operator=(Image&& other) noexcept {
		if (this != &other) {
			if (m_data) stbi_image_free(m_data);
			m_data = other.m_data;
			m_dimension = other.m_dimension;
			m_channelCount = other.m_channelCount;
			m_loaded = other.m_loaded;
			m_valid = other.m_valid;

			other.m_data = nullptr;
			other.m_valid = false;
		}
		return *this;
	}

	u8* data() const { return m_data; };
	Coord dimension() const { return m_dimension; }
	int channelCount() const { return m_channelCount; };
	bool valid() const { return m_valid; }

	u32 size() const { return m_dimension.x * m_dimension.y * 4; } // Forced 4 channels
	std::span<u8> getSpan() const { return std::span<u8>(m_data, size()); }

private:
	u8* m_data = nullptr;
	Coord m_dimension{};
	int m_channelCount = 0;
	bool m_loaded = 0, m_valid = 0;
};
} // namespace tx