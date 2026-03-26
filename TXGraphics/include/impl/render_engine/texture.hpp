// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: texture_manager.hpp

#pragma once
#include "impl/render_engine/basic_gl_utils.hpp"
#include <concepts>
#include <span>

namespace tx::RenderEngine {

enum class TextureFormat : u32 {
	Grey = gl::enums::R8,
	RGBA = gl::enums::RGBA8,
	RGB = gl::enums::RGB8
};

enum class TextureRule : u32 {
	Pixel = gl::enums::NEAREST,
	Linear = gl::enums::LINEAR,
	Repeat = gl::enums::REPEAT,
	Clamp = gl::enums::CLAMP_TO_EDGE
};

inline u32 glCreateTexture() {
	u32 id;
	gl::createTextures(gl::enums::TEXTURE_2D_ARRAY, 1, &id);
	return id;
}

struct TextureDeleter {
	gid id = 0;
	u64 handle = 0;
	void operator()() const {
		if (handle) gl::makeTextureHandleNonResidentARB(handle);
		if (id) gl::deleteTextures(1, &id);
	}
};

// using GL_ARB_bindless_texture
class TextureArray {
public:
	TextureArray() {}
	TextureArray(
	    TextureFormat format,
	    Coord dimension, u32 layerCount,
	    bool useMipmap,
	    TextureRule scaleRule = TextureRule::Linear,
	    TextureRule xWrapRule = TextureRule::Clamp,
	    TextureRule yWrapRule = TextureRule::Clamp,
	    u8* data = nullptr);

	~TextureArray() {
		if (m_handle) gl::makeTextureHandleNonResidentARB(m_handle);
		if (m_id) gl::deleteTextures(1, &m_id);
	}

	// Disable Copy
	TextureArray(const TextureArray&) = delete;
	TextureArray& operator=(const TextureArray&) = delete;

	// Move Semantics
	TextureArray(TextureArray&& other) noexcept
	    : m_id(other.m_id),
	      m_handle(other.m_handle),
	      m_format(other.m_format),
	      m_scaleRule(other.m_scaleRule),
	      m_xWrapRule(other.m_xWrapRule),
	      m_yWrapRule(other.m_yWrapRule),
	      m_dimension(other.m_dimension),
	      m_layerCount(other.m_layerCount),
	      m_useMipmap(other.m_useMipmap) {
		other.m_id = 0;
		other.m_handle = 0;
	}
	TextureArray& operator=(TextureArray&& other) noexcept {
		if (this != &other) {
			if (m_handle) gl::makeTextureHandleNonResidentARB(m_handle);
			if (m_id) gl::deleteTextures(1, &m_id);
			m_id = other.m_id;
			m_handle = other.m_handle;
			m_format = other.m_format;
			m_scaleRule = other.m_scaleRule;
			m_xWrapRule = other.m_xWrapRule;
			m_yWrapRule = other.m_yWrapRule;
			m_dimension = other.m_dimension;
			m_layerCount = other.m_layerCount;
			m_useMipmap = other.m_useMipmap;
			other.m_id = 0;
			other.m_handle = 0;
		}
		return *this;
	}

	void setLayer(u32 layer, std::span<u8> data) {
		setLayerRegion_impl(layer, tx::CoordOrigin, m_dimension, data.data());
	}
	void setLayerRegion(u32 layer, Coord offset, Coord dimension, std::span<u8> data) {
		setLayerRegion_impl(layer, offset, dimension, data.data());
	}

	template <class Func>
	    requires std::invocable<Func, TextureDeleter&&>
	void resize(u32 newLayerCount, Func&& submitDeleter) {
		if (newLayerCount <= m_layerCount) return;
		submitDeleter(resize_impl(newLayerCount));
	}


	gid id() const { return m_id; }
	u64 handle() const { return m_handle; }
	u32 size() const { return m_layerCount; }
	Coord dimension() const { return m_dimension; }


private:
	gid m_id = 0;
	u64 m_handle = 0;
	TextureFormat m_format = TextureFormat::RGBA;
	TextureRule m_scaleRule = TextureRule::Linear;
	TextureRule m_xWrapRule = TextureRule::Clamp;
	TextureRule m_yWrapRule = TextureRule::Clamp;
	Coord m_dimension = { 0, 0 };
	u32 m_layerCount = 1;
	bool m_useMipmap = false;

	[[nodiscard]] TextureDeleter resize_impl(u32 newLayerCount);

	void alloc_impl(TextureFormat format, Coord dimension, u32 mipmapLevel, u32 layerCount) {
		gl::textureStorage3D(m_id, mipmapLevel, getFormatInternal_impl(format), dimension.x(), dimension.y(), layerCount);
	}
	void setLayerRegion_impl(u32 layer, Coord offset, Coord dimension, u8* data) {
		gl::textureSubImage3D(
		    m_id, 0,
		    offset.x(), offset.y(), layer,
		    dimension.x(), dimension.y(), 1,
		    getFormat_impl(m_format), gl::enums::UNSIGNED_BYTE, data);
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
		case TextureFormat::Grey: return gl::enums::RED;
		case TextureFormat::RGBA: return gl::enums::RGBA;
		case TextureFormat::RGB: return gl::enums::RGB;
		default: return 0;
		}
	}
	u32 getFormatInternal_impl(TextureFormat format) { return enumval(format); }
	void setRules_impl(TextureRule scaleRule, TextureRule xWrapRule, TextureRule yWrapRule) {
		gl::textureParameteri(m_id, gl::enums::TEXTURE_MAG_FILTER, enumval(scaleRule));
		gl::textureParameteri(m_id, gl::enums::TEXTURE_MIN_FILTER, enumval(scaleRule));
		gl::textureParameteri(m_id, gl::enums::TEXTURE_WRAP_S, enumval(xWrapRule));
		gl::textureParameteri(m_id, gl::enums::TEXTURE_WRAP_T, enumval(yWrapRule));
	}
};



class TextureArray_legacy {
public:
	TextureArray_legacy() {}
	TextureArray_legacy(TextureFormat format, Coord dimension, u32 layerCount, bool useMipmap, u8* data = nullptr);

	~TextureArray_legacy() {
		if (m_id) gl::deleteTextures(1, &m_id);
	}

	// Disable Copy
	TextureArray_legacy(const TextureArray_legacy&) = delete;
	TextureArray_legacy& operator=(const TextureArray_legacy&) = delete;

	// Move Semantics
	TextureArray_legacy(TextureArray_legacy&& other) noexcept
	    : m_id(other.m_id),
	      m_format(other.m_format),
	      m_scaleRule(other.m_scaleRule),
	      m_xWrapRule(other.m_xWrapRule),
	      m_yWrapRule(other.m_yWrapRule),
	      m_dimension(other.m_dimension),
	      m_layerCount(other.m_layerCount),
	      m_useMipmap(other.m_useMipmap) {
		other.m_id = 0;
	}
	TextureArray_legacy& operator=(TextureArray_legacy&& other) noexcept {
		if (this != &other) {
			if (m_id) gl::deleteTextures(1, &m_id);
			m_id = other.m_id;
			m_format = other.m_format;
			m_scaleRule = other.m_scaleRule;
			m_xWrapRule = other.m_xWrapRule;
			m_yWrapRule = other.m_yWrapRule;
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

	template <class Func>
	    requires std::invocable<Func, TextureDeleter&&>
	void resize(u32 newLayerCount, Func&& submitDeleter) {
		if (newLayerCount <= m_layerCount) return;
		submitDeleter(resize_impl(newLayerCount));
	}

	void setScaleRule(TextureRule rule) {
		m_scaleRule = rule;
		if (rule == TextureRule::Linear || rule == TextureRule::Pixel) {
			gl::textureParameteri(m_id, gl::enums::TEXTURE_MAG_FILTER, enumval(rule));
			gl::textureParameteri(m_id, gl::enums::TEXTURE_MIN_FILTER, enumval(rule));
		}
	}
	void setXWrapRule(TextureRule rule) {
		m_xWrapRule = rule;
		if (rule == TextureRule::Clamp || rule == TextureRule::Repeat) {
			gl::textureParameteri(m_id, gl::enums::TEXTURE_WRAP_S, enumval(rule));
		}
	}
	void setYWrapRule(TextureRule rule) {
		m_yWrapRule = rule;
		if (rule == TextureRule::Clamp || rule == TextureRule::Repeat) {
			gl::textureParameteri(m_id, gl::enums::TEXTURE_WRAP_T, enumval(rule));
		}
	}
	void setWrapRule(TextureRule rule) {
		m_xWrapRule = rule;
		m_yWrapRule = rule;
		if (rule == TextureRule::Clamp || rule == TextureRule::Repeat) {
			gl::textureParameteri(m_id, gl::enums::TEXTURE_WRAP_S, enumval(rule));
			gl::textureParameteri(m_id, gl::enums::TEXTURE_WRAP_T, enumval(rule));
		}
	}
	void setTextureRule(TextureRule scaleRule, TextureRule wrapRule) {
		setScaleRule(scaleRule);
		setWrapRule(wrapRule);
	}
	void setRule(TextureRule scaleRule, TextureRule wrapRule) { setTextureRule(scaleRule, wrapRule); }

	gid id() const { return m_id; }
	u32 size() const { return m_layerCount; }
	Coord dimension() const { return m_dimension; }


private:
	gid m_id = 0;
	TextureFormat m_format = TextureFormat::RGBA;
	TextureRule m_scaleRule = TextureRule::Linear, m_xWrapRule = TextureRule::Clamp, m_yWrapRule = TextureRule::Clamp;
	Coord m_dimension = { 0, 0 };
	u32 m_layerCount = 1;
	bool m_useMipmap = false;

	[[nodiscard]] TextureDeleter resize_impl(u32 newLayerCount);

	void alloc_impl(TextureFormat format, Coord dimension, u32 mipmapLevel, u32 layerCount) {
		gl::textureStorage3D(m_id, mipmapLevel, getFormatInternal_impl(format), dimension.x(), dimension.y(), layerCount);
	}
	void setLayerRegion_impl(u32 layer, Coord offset, Coord dimension, u8* data) {
		gl::textureSubImage3D(
		    m_id, 0,
		    offset.x(), offset.y(), layer,
		    dimension.x(), dimension.y(), 1,
		    getFormat_impl(m_format), gl::enums::UNSIGNED_BYTE, data);
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
		case TextureFormat::Grey: return gl::enums::RED;
		case TextureFormat::RGBA: return gl::enums::RGBA;
		case TextureFormat::RGB: return gl::enums::RGB;
		default: return 0;
		}
	}
	u32 getFormatInternal_impl(TextureFormat format) { return enumval(format); }
};



} // namespace tx::RenderEngine