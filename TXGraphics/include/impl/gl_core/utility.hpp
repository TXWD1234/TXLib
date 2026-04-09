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
	BufferT bo; // Buffer Object
};

template <class BufferT>
inline void VAMSetBuffer(VAM& vam, const BufferHandle<BufferT>& handle, u32 offset = 0) {
	vam.setBuffer(handle.id, handle.bo, offset);
}
template <class T, class SubmitMarker>
    requires std::invocable<SubmitMarker, RingBufferObjectMarker&&>
inline void VAMUpdateRingBuffer(VAM& vam, BufferHandle<RingBufferObject<T>>& handle, SubmitMarker&& submitMarker) {
	vam.setBuffer(handle.id, handle.bo, handle.bo.getNext(std::forward<SubmitMarker>(submitMarker)));
}

// sm

template <SMStyle style>
bool addShaderPair(ShaderManager<style>& sm, const std::string& vertSource, const std::string& fragSource, ProgramId& output, const std::string& vertName = "", const std::string& fragName = "") {
	ShaderId vertId = sm.addVertexShader(vertSource);
	std::cerr << "Compiling & Linking vertex shader... " << vertName << '\n'
	          << sm.getCompileLog(vertId) << "Done.\n";
	if (!sm.compileSucceed(vertId)) return 0;

	ShaderId fragId = sm.addFragmentShader(fragSource);
	std::cerr << "Compiling & Linking fragment shader... " << fragName << '\n'
	          << sm.getCompileLog(fragId) << "Done.\n";
	if (!sm.compileSucceed(fragId)) return 0;

	output = sm.linkShaders(vertId, fragId);
	std::cerr << "Linking shaders... " << vertName << ' ' << fragName << '\n'
	          << sm.getLinkLog(output) << "Done." << std::endl;
	if (!sm.linkSucceed(output)) return 0;

	return 1;
}


} // namespace tx::RenderEngine