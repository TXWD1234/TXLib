// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: texture.cpp

#include "impl/render_engine/texture.hpp"

namespace tx::RenderEngine {

TextureArray::TextureArray(
    TextureFormat format,
    Coord dimension, u32 layerCount,
    bool useMipmap,
    TextureRule scaleRule,
    TextureRule xWrapRule,
    TextureRule yWrapRule,
    u8* data)
    : m_format(format), m_dimension(dimension), m_layerCount(layerCount), m_useMipmap(useMipmap),
      m_scaleRule(scaleRule), m_xWrapRule(xWrapRule), m_yWrapRule(yWrapRule) {
	gl::createTextures(gl::enums::TEXTURE_2D_ARRAY, 1, &m_id);
	init_impl();
	if (data != nullptr) { // assign data
		gl::textureSubImage3D(
		    m_id, 0,
		    0, 0, 0,
		    m_dimension.x(), m_dimension.y(), m_layerCount,
		    getFormat_impl(m_format), gl::enums::UNSIGNED_BYTE, data);
	}
	// set rules
	setRules_impl(scaleRule, xWrapRule, yWrapRule);
	// get handle
	m_handle = gl::getTextureHandleARB(m_id);
	gl::makeTextureHandleResidentARB(m_handle);
}
void TextureArray::resize(u32 newLayerCount) {
	if (newLayerCount <= m_layerCount) return;
	gid oldId = m_id;
	u32 oldLayerCount = m_layerCount;
	m_layerCount = newLayerCount;
	gl::createTextures(gl::enums::TEXTURE_2D_ARRAY, 1, &m_id);
	init_impl();
	gl::copyImageSubData(
	    oldId, gl::enums::TEXTURE_2D_ARRAY, 0, 0, 0, 0,
	    m_id, gl::enums::TEXTURE_2D_ARRAY, 0, 0, 0, 0,
	    m_dimension.x(), m_dimension.y(), oldLayerCount);
	setRules_impl(m_scaleRule, m_xWrapRule, m_yWrapRule);

	if (m_handle) gl::makeTextureHandleNonResidentARB(m_handle);
	gl::deleteTextures(1, &oldId);
	m_handle = gl::getTextureHandleARB(m_id);
	gl::makeTextureHandleResidentARB(m_handle);
}

TextureArray_legacy::TextureArray_legacy(TextureFormat format, Coord dimension, u32 layerCount, bool useMipmap, u8* data)
    : m_format(format), m_dimension(dimension), m_layerCount(layerCount), m_useMipmap(useMipmap) {
	gl::createTextures(gl::enums::TEXTURE_2D_ARRAY, 1, &m_id);
	init_impl();
	if (data != nullptr) { // assign data
		gl::textureSubImage3D(
		    m_id, 0,
		    0, 0, 0,
		    m_dimension.x(), m_dimension.y(), m_layerCount,
		    getFormat_impl(m_format), gl::enums::UNSIGNED_BYTE, data);
	}
}
void TextureArray_legacy::resize(u32 newLayerCount) {
	if (newLayerCount <= m_layerCount) return;
	gid oldId = m_id;
	u32 oldLayerCount = m_layerCount;
	m_layerCount = newLayerCount;
	gl::createTextures(gl::enums::TEXTURE_2D_ARRAY, 1, &m_id);
	init_impl();
	gl::copyImageSubData(
	    oldId, gl::enums::TEXTURE_2D_ARRAY, 0, 0, 0, 0,
	    m_id, gl::enums::TEXTURE_2D_ARRAY, 0, 0, 0, 0,
	    m_dimension.x(), m_dimension.y(), oldLayerCount);
	setScaleRule(m_scaleRule);
	setXWrapRule(m_xWrapRule);
	setYWrapRule(m_yWrapRule);
	gl::deleteTextures(1, &oldId);
}

} // namespace tx::RenderEngine