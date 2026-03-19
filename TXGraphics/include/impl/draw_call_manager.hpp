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
template <ShaderManageStyle shaderManType = ShaderManageStyle::Program>
class DrawCallManager {
public:
	DrawCallManager() {}
	DrawCallManager(ShaderManager<shaderManType>& in_sm, VertexAttributeManager& in_vam)
	    : m_sm(&in_sm), m_vam(&in_vam) {
	}

	// Disable Copy
	DrawCallManager(const DrawCallManager&) = delete;
	DrawCallManager& operator=(const DrawCallManager&) = delete;
	DrawCallManager(DrawCallManager&& other) noexcept
	    : m_sm(other.m_sm),
	      m_vam(other.m_vam),
	      m_shaders(other.m_shaders) {
		other.m_sm = nullptr;
		other.m_vam = nullptr;
		other.m_shaders = { 0, 0 };
	}
	DrawCallManager& operator=(DrawCallManager&& other) noexcept {
		if (this != &other) {
			m_sm = other.m_sm;
			m_vam = other.m_vam;
			m_shaders = other.m_shaders;

			other.m_sm = nullptr;
			other.m_vam = nullptr;
			other.m_shaders = { 0, 0 };
		}
		return *this;
	}

	void draw(u32 begin, u32 count) {
		bindVAM_impl();
		bindShader_impl();
		drawArrays_impl(begin, count);
	}

	void drawInstanced(u32 begin, u32 count, u32 instanceCount) {
		bindVAM_impl();
		bindShader_impl();
		drawArraysInstanced_impl(begin, count, instanceCount);
	}

	void setShaders(ShaderPair shaders) { m_shaders = shaders; }
	void setVertexShader(ShaderId vertId) { m_shaders.vertId = vertId.id; }
	void setFragementShader(ShaderId fragId) { m_shaders.fragId = fragId.id; }
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

	ShaderManager<shaderManType>* m_sm = nullptr;
	VertexAttributeManager* m_vam = nullptr;
	ShaderPair m_shaders = { 0, 0 };

	inline static thread_local gid currentBindingVAM = 0;
	inline static thread_local gid currentBindingShaderPipeline = 0;
	inline static thread_local gid currentBindingShaderProgram = 0;

	void bindVAM_impl() {
		if (currentBindingVAM != m_vam->id()) {
			glBindVertexArray(m_vam->id());
			currentBindingVAM = m_vam->id();
		}
	}

	void bindShader_impl() {
		if constexpr (shaderManType == ShaderManageStyle::Pipeline) {
			if (currentBindingShaderProgram != 0) {
				glUseProgram(0);
				currentBindingShaderProgram = 0;
			}
			gid id = m_sm->getPipelineId(m_shaders);
			if (id != currentBindingShaderPipeline) {
				currentBindingShaderPipeline = id;
				glBindProgramPipeline(id);
			}
		} else { // shader program
			gid id = m_sm->getProgramId(m_shaders);
			if (id != currentBindingShaderProgram) {
				currentBindingShaderProgram = id;
				glUseProgram(id);
			}
		}
	}
};
template <ShaderManageStyle shaderManType = ShaderManageStyle::Program>
using DCM = DrawCallManager<shaderManType>;
} // namespace RenderEngine
} // namespace tx
