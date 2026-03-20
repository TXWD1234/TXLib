// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: draw_call_manager.hpp

#pragma once
#include "impl/basic_gl_utils.hpp"
#include "impl/vertex_attribute_manager.hpp"
#include "impl/shader_manager.hpp"
#include "impl/texture_manager.hpp"

namespace tx {
namespace RenderEngine {
class DrawCallManager {
public:
	DrawCallManager() {}
	DrawCallManager(VertexAttributeManager& in_vam)
	    : m_vam(&in_vam) {
	}

	// Disable Copy
	DrawCallManager(const DrawCallManager&) = delete;
	DrawCallManager& operator=(const DrawCallManager&) = delete;
	DrawCallManager(DrawCallManager&& other) noexcept
	    : m_vam(other.m_vam) {
		other.m_vam = nullptr;
	}
	DrawCallManager& operator=(DrawCallManager&& other) noexcept {
		if (this != &other) {
			m_vam = other.m_vam;

			other.m_vam = nullptr;
		}
		return *this;
	}

	// draw call

	void draw(u32 begin, u32 count) {
		bindVAM_impl();
		drawArrays_impl(begin, count);
	}

	void drawInstanced(u32 begin, u32 count, u32 instanceCount) {
		bindVAM_impl();
		drawArraysInstanced_impl(begin, count, instanceCount);
	}

	// configure

	void setShaderProgram(const ShaderProgram& program) {
		gid id = program.id();
		if (id != currentBindingShaderProgram) {
			currentBindingShaderProgram = id;
			glUseProgram(id);
		}
	}
	void setShaderPipeline(const ShaderPipeline& pipeline) {
		if (currentBindingShaderProgram != 0) {
			glUseProgram(0);
			currentBindingShaderProgram = 0;
		}
		gid id = pipeline.id();
		if (id != currentBindingShaderPipeline) {
			currentBindingShaderPipeline = id;
			glBindProgramPipeline(id);
		}
	}
	// should i add valid check in these two functions?

	void setTexture(u32 port, const TextureArray& tex) {
		glBindTextureUnit(port, tex.id());
	}


private:
	void drawArrays_impl(u32 begin, u32 count) const {
		glDrawArrays(GL_TRIANGLES, begin, count);
	}
	void drawArraysInstanced_impl(u32 begin, u32 count, u32 instanceCount) const {
		glDrawArraysInstanced(GL_TRIANGLES, begin, count, instanceCount);
	}

	VertexAttributeManager* m_vam = nullptr;

	inline static thread_local gid currentBindingVAM = 0;
	inline static thread_local gid currentBindingShaderPipeline = 0;
	inline static thread_local gid currentBindingShaderProgram = 0;

	void bindVAM_impl() {
		if (currentBindingVAM != m_vam->id()) {
			glBindVertexArray(m_vam->id());
			currentBindingVAM = m_vam->id();
		}
	}
};
using DCM = DrawCallManager;
} // namespace RenderEngine
} // namespace tx
