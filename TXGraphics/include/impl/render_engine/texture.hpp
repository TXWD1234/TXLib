// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: texture_manager.hpp

#pragma once
#include "impl/render_engine/basic_gl_utils.hpp"
#include <span>

namespace tx::RenderEngine {

enum class TextureFormat : u32 {
	Grey = 0x8229, // GL_R8
	RGBA = 0x8058, // GL_RGBA8
	RGB = 0x8051 // GL_RGB8
};

enum class TextureRule : u32 {
	Pixel = 0x2600, // GL_NEAREST
	Linear = 0x2601, // GL_LINEAR
	Repeat = 0x2901, // GL_REPEAT
	Clamp = 0x812F // GL_CLAMP_TO_EDGE
};

inline u32 glCreateTexture() {
	u32 id;
	gl::createTextures(0x8C1A /* GL_TEXTURE_2D_ARRAY */, 1, &id);
	return id;
}

class TextureArray {
public:
	TextureArray() {}
	TextureArray(TextureFormat format, Coord dimension, u32 layerCount, bool useMipmap, u8* data = nullptr);

	~TextureArray() {
		if (m_id) gl::deleteTextures(1, &m_id);
	}

	// Disable Copy
	TextureArray(const TextureArray&) = delete;
	TextureArray& operator=(const TextureArray&) = delete;

	// Move Semantics
	TextureArray(TextureArray&& other) noexcept
	    : m_id(other.m_id),
	      m_format(other.m_format),
	      m_dimension(other.m_dimension),
	      m_layerCount(other.m_layerCount),
	      m_useMipmap(other.m_useMipmap) {
		other.m_id = 0;
	}
	TextureArray& operator=(TextureArray&& other) noexcept {
		if (this != &other) {
			if (m_id) gl::deleteTextures(1, &m_id);
			m_id = other.m_id;
			m_format = other.m_format;
			m_dimension = other.m_dimension;
			m_layerCount = other.m_layerCount;
			m_useMipmap = other.m_useMipmap;
			other.m_id = 0;
		}
		return *this;
	}

	void setLayer(u32 layer, std::span<u8> data) {
		setLayerRegion_impl(layer, tx::CoordOrigin, m_dimension, data.data());
	}
	void setLayerRegion(u32 layer, Coord offset, Coord dimension, std::span<u8> data) {
		setLayerRegion_impl(layer, offset, dimension, data.data());
	}

	void setScaleRule(TextureRule rule) {
		if (rule == TextureRule::Linear || rule == TextureRule::Pixel) {
			gl::textureParameteri(m_id, 0x2800 /* GL_TEXTURE_MAG_FILTER */, enumval(rule));
			gl::textureParameteri(m_id, 0x2801 /* GL_TEXTURE_MIN_FILTER */, enumval(rule));
		}
	}
	void setXWrapRule(TextureRule rule) {
		if (rule == TextureRule::Clamp || rule == TextureRule::Repeat) {
			gl::textureParameteri(m_id, 0x2802 /* GL_TEXTURE_WRAP_S */, enumval(rule));
		}
	}
	void setYWrapRule(TextureRule rule) {
		if (rule == TextureRule::Clamp || rule == TextureRule::Repeat) {
			gl::textureParameteri(m_id, 0x2803 /* GL_TEXTURE_WRAP_T */, enumval(rule));
		}
	}
	void setWrapRule(TextureRule rule) {
		if (rule == TextureRule::Clamp || rule == TextureRule::Repeat) {
			gl::textureParameteri(m_id, 0x2802 /* GL_TEXTURE_WRAP_S */, enumval(rule));
			gl::textureParameteri(m_id, 0x2803 /* GL_TEXTURE_WRAP_T */, enumval(rule));
		}
	}
	void setTextureRule(TextureRule scaleRule, TextureRule wrapRule) {
		setScaleRule(scaleRule);
		setWrapRule(wrapRule);
	}

	gid id() const { return m_id; }
	u32 size() const { return m_layerCount; }
	Coord dimension() const { return m_dimension; }


private:
	gid m_id = 0;
	TextureFormat m_format = TextureFormat::RGBA;
	Coord m_dimension = { 0, 0 };
	u32 m_layerCount = 1;
	bool m_useMipmap = false;

	void alloc_impl(TextureFormat format, Coord dimension, u32 mipmapLevel, u32 layerCount) {
		gl::textureStorage3D(m_id, mipmapLevel, getFormatInternal_impl(format), dimension.x(), dimension.y(), layerCount);
	}
	void setLayerRegion_impl(u32 layer, Coord offset, Coord dimension, u8* data) {
		gl::textureSubImage3D(
		    m_id, 0,
		    offset.x(), offset.y(), layer,
		    dimension.x(), dimension.y(), 1,
		    getFormat_impl(m_format), 0x1401 /* GL_UNSIGNED_BYTE */, data);
	}
	void init_impl() {
		u32 mipmapLevel = m_useMipmap ? getMipmapLevel_impl(m_dimension) : 1;
		alloc_impl(m_format, m_dimension, mipmapLevel, m_layerCount);
	}
	u32 getMipmapLevel_impl(Coord dimension) {
		return 1; // reserve for later
	}
	u32 getFormat_impl(TextureFormat format) {
		switch (format) {
		case TextureFormat::Grey: return 0x1903 /* GL_RED */;
		case TextureFormat::RGBA: return 0x1908 /* GL_RGBA */;
		case TextureFormat::RGB: return 0x1907 /* GL_RGB */;
		default: return 0;
		}
	}
	u32 getFormatInternal_impl(TextureFormat format) { return enumval(format); }
};



} // namespace tx::RenderEngine