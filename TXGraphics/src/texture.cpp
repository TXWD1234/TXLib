// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: texture.cpp

#include "impl/texture.hpp"

namespace tx::RenderEngine {

TextureArray::TextureArray(TextureFormat format, Coord dimension, u32 layerCount, bool useMipmap, u8* data)
    : m_format(format), m_dimension(dimension), m_layerCount(layerCount), m_useMipmap(useMipmap) {
	glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_id);
	init_impl();
	if (data != nullptr) { // assign data
		glTextureSubImage3D(
		    m_id, 0,
		    0, 0, 0,
		    m_dimension.x(), m_dimension.y(), m_layerCount,
		    getFormat_impl(m_format), GL_UNSIGNED_BYTE, data);
	}
}

} // namespace tx::RenderEngine