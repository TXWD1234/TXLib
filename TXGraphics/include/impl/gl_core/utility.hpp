// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: utility.hpp

#pragma once
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/shader.hpp"
#include "impl/gl_core/shader_manager.hpp"
#include "impl/gl_core/buffer.hpp"
#include "impl/gl_core/vertex_attribute_manager.hpp"
#include "impl/gl_core/fence_manager.hpp"
#include "impl/gl_core/texture.hpp"

#include <iostream>
#include <utility>


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
template <class T, class SubmitMarker>
    requires std::invocable<SubmitMarker, RingBufferObjectMarker&&>
inline void VAMUpdateRingBuffer(VAM& vam, BufferHandle<RingBufferObject<T>>& handle, SubmitMarker&& submitMarker) {
	vam.setBuffer(handle.id, handle.bo, handle.bo.getNext(std::forward<SubmitMarker>(submitMarker)));
}

// fence


// using FenceManager_t = FenceManager<
//     RingBufferObjectDeleter,
//     RingBufferObjectMarker,
//     TextureDeleter>;


} // namespace tx::RenderEngine