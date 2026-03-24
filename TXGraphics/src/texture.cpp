// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: texture.cpp

#include "impl/render_engine/texture.hpp"

namespace tx::RenderEngine {

TextureArray::TextureArray(TextureFormat format, Coord dimension, u32 layerCount, bool useMipmap, u8* data)
    : m_format(format), m_dimension(dimension), m_layerCount(layerCount), m_useMipmap(useMipmap) {
	gl::createTextures(gl::TEXTURE_2D_ARRAY, 1, &m_id);
	init_impl();
	if (data != nullptr) { // assign data
		gl::textureSubImage3D(
		    m_id, 0,
		    0, 0, 0,
		    m_dimension.x(), m_dimension.y(), m_layerCount,
		    getFormat_impl(m_format), gl::UNSIGNED_BYTE, data);
	}
}

} // namespace tx::RenderEngine