// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: render_engine.hpp

#pragma once
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/buffer.hpp"
#include "impl/gl_core/vertex_attribute_manager.hpp"
#include "impl/gl_core/shader_manager.hpp"
#include "impl/gl_core/draw_call_manager.hpp"
#include "impl/gl_core/fence_manager.hpp"
#include "impl/gl_core/utility.hpp"

#include "impl/render_engine/texture_manager.hpp"


namespace tx::RenderEngine {

class RenderEngine {
public:
	void draw() {
	}



public:
private:
	class BufferManager {
	public:
	private:
		StaticBufferObject<vec2> meshPositionBuffer;
		StaticBufferObject<vec2> meshUVBuffer;
		RingBufferObject<vec2> instancePositionBuffer;
		RingBufferObject<float> instancePositionBufferaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa; // aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
		RingBufferObject<float> instanceTextureIndexBuffer;
	};

private:
	DrawCallManager dcm;
	ShaderManager<SMStyle::Program> sm;
	VertexAttributeManager vam;
	//TextureManager tm;
};
} // namespace tx::RenderEngine