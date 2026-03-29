// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: render_engine.hpp

#pragma once
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/shader_manager.hpp"
#include "impl/render_engine/texture_manager.hpp"
#include "impl/gl_core/utility.hpp"

#include "impl/render_engine/renderer.hpp"

namespace tx::RenderEngine {

class RenderEngine {
public:
	void draw() {
	}



public:
private:
private:
	ShaderManager<SMStyle::Program> sm;
	//TextureManager tm;
};
} // namespace tx::RenderEngine