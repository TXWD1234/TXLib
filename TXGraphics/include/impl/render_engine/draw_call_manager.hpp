// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: draw_call_manager.hpp

#pragma once
#include "impl/render_engine/basic_gl_utils.hpp"
#include "impl/render_engine/vertex_attribute_manager.hpp"
#include "impl/render_engine/shader_manager.hpp"
#include "impl/render_engine/texture.hpp"
#include <vector>

namespace tx {
namespace RenderEngine {
class DrawCallManager {
public:
	DrawCallManager() = default;

	// Disable Copy
	DrawCallManager(const DrawCallManager&) = delete;
	DrawCallManager& operator=(const DrawCallManager&) = delete;
	DrawCallManager(DrawCallManager&& other) noexcept
	    : m_begin(other.m_begin),
	      m_count(other.m_count),
	      m_instanceCount(other.m_instanceCount),
	      m_vamId(other.m_vamId),
	      m_programId(other.m_programId),
	      m_pipelineId(other.m_pipelineId),
	      m_usePipeline(other.m_usePipeline),
	      m_textures(std::move(other.m_textures)) {
		other.m_vamId = 0;
		other.m_programId = 0;
		other.m_pipelineId = 0;
	}
	DrawCallManager& operator=(DrawCallManager&& other) noexcept {
		if (this != &other) {
			m_begin = other.m_begin;
			m_count = other.m_count;
			m_instanceCount = other.m_instanceCount;
			m_vamId = other.m_vamId;
			m_programId = other.m_programId;
			m_pipelineId = other.m_pipelineId;
			m_usePipeline = other.m_usePipeline;
			m_textures = std::move(other.m_textures);
			other.m_vamId = 0;
			other.m_programId = 0;
			other.m_pipelineId = 0;
		}
		return *this;
	}

	// draw call

	void draw() {
		bindVAM_impl();
		bindShaders_impl();
		bindTextures_impl();
		drawArrays_impl(m_begin, m_count);
	}

	void draw(u32 begin, u32 count) {
		bindVAM_impl();
		bindShaders_impl();
		bindTextures_impl();
		drawArrays_impl(begin, count);
	}

	void drawInstanced() {
		bindVAM_impl();
		bindShaders_impl();
		bindTextures_impl();
		drawArraysInstanced_impl(m_begin, m_count, m_instanceCount);
	}

	void drawInstanced(u32 begin, u32 count, u32 instanceCount) {
		bindVAM_impl();
		bindShaders_impl();
		bindTextures_impl();
		drawArraysInstanced_impl(begin, count, instanceCount);
	}

	// configure

	void setBegin(u32 begin) { m_begin = begin; }
	void setCount(u32 count) { m_count = count; }
	void setInstanceCount(u32 instanceCount) { m_instanceCount = instanceCount; }

	void setDrawCallParameters(u32 begin, u32 count, u32 instanceCount) {
		m_begin = begin;
		m_count = count;
		m_instanceCount = instanceCount;
	}
	void setDrawCallParameters(u32 begin, u32 count) {
		m_begin = begin;
		m_count = count;
	}
	void setParameters(u32 begin, u32 count, u32 instanceCount) { setDrawCallParameters(begin, count, instanceCount); }
	void setParameters(u32 begin, u32 count) { setDrawCallParameters(begin, count); }

	void setShaderProgram(const ShaderProgram& program) {
		m_programId = program.id();
		m_usePipeline = false;
	}
	void setShaderPipeline(const ShaderPipeline& pipeline) {
		m_pipelineId = pipeline.id();
		m_usePipeline = true;
	}
	void setShaders(const ShaderProgram& program) {
		setShaderProgram(program);
	}
	void setShaders(const ShaderPipeline& pipeline) {
		setShaderPipeline(pipeline);
	}

	void setTexture(u32 port, const TextureArray_legacy& tex) {
		if (port >= m_textures.size()) m_textures.resize(port + 1, 0);
		m_textures[port] = tex.id();
	}

	void setVertexAttributeManager(const VAM& in_vam) {
		m_vamId = in_vam.id();
	}
	void setVAM(const VAM& in_vam) {
		m_vamId = in_vam.id();
	}


private:
	void drawArrays_impl(u32 begin, u32 count) const {
		gl::drawArrays(gl::enums::TRIANGLES, begin, count);
	}
	void drawArraysInstanced_impl(u32 begin, u32 count, u32 instanceCount) const {
		gl::drawArraysInstanced(gl::enums::TRIANGLES, begin, count, instanceCount);
	}

	// draw call variables

	u32 m_begin = 0, m_count = 0, m_instanceCount = 0;

	gid m_vamId = 0;
	gid m_programId = 0;
	gid m_pipelineId = 0;
	bool m_usePipeline = false;
	std::vector<gid> m_textures;

	inline static thread_local gid currentBindingVAM = 0;
	inline static thread_local gid currentBindingShaderPipeline = 0;
	inline static thread_local gid currentBindingShaderProgram = 0;
	inline static thread_local std::vector<gid> currentBindingTextures;

	void bindVAM_impl() {
		if (m_vamId && currentBindingVAM != m_vamId) {
			gl::bindVertexArray(m_vamId);
			currentBindingVAM = m_vamId;
		}
	}

	void bindShaders_impl() {
		if (m_usePipeline && m_pipelineId) {
			if (currentBindingShaderProgram != 0) {
				gl::useProgram(0);
				currentBindingShaderProgram = 0;
			}
			if (m_pipelineId != currentBindingShaderPipeline) {
				currentBindingShaderPipeline = m_pipelineId;
				gl::bindProgramPipeline(m_pipelineId);
			}
		} else if (!m_usePipeline && m_programId) {
			if (m_programId != currentBindingShaderProgram) {
				currentBindingShaderProgram = m_programId;
				gl::useProgram(m_programId);
			}
		}
	}

	void bindTextures_impl() {
		for (u32 i = 0; i < m_textures.size(); ++i) {
			gid id = m_textures[i];
			if (id != 0) {
				if (i >= currentBindingTextures.size()) {
					currentBindingTextures.resize(i + 1, 0);
				}
				if (currentBindingTextures[i] != id) {
					gl::bindTextureUnit(i, id);
					currentBindingTextures[i] = id;
				}
			}
		}
	}
};
using DCM = DrawCallManager;
} // namespace RenderEngine
} // namespace tx
