// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: utility.hpp

#pragma once
#include "impl/render_engine/basic_gl_utils.hpp"
#include "impl/render_engine/shader.hpp"
#include "impl/render_engine/shader_manager.hpp"
#include "impl/render_engine/buffer.hpp"
#include "impl/render_engine/vertex_attribute_manager.hpp"

#include <filesystem>
#include <iostream>


namespace tx::RenderEngine {

// vam related

template <class BufferT>
struct BufferHandle {
	u32 id; // id in vam
	BufferT bo; // Buffer Obejct
};

template <class BufferT>
inline void VAMBindBuffer(VAM& vam, const BufferHandle<BufferT>& handle, u32 offset = 0) {
	vam.setBuffer(handle.id, handle.bo, offset);
}
template <class T>
inline void VAMUpdateRingBuffer(VAM& vam, BufferHandle<RingBufferObject<T>>& handle) {
	vam.setBuffer(handle.id, handle.bo, handle.bo.getNext());
}



} // namespace tx::RenderEngine