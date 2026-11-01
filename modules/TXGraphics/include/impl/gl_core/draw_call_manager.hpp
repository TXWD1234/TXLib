// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXGraphics

#pragma once
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/shader.hpp"
#include "impl/gl_core/vertex_attribute_manager.hpp"
#include "tx/upset.hpp"

namespace tx {
namespace RenderEngine {

// designed for SSBO oriented, single shader draw calls with instancing
// targeted to be as basic and as simple as possible
class SimpleDrawCallManager {
public:
	SimpleDrawCallManager(const ShaderProgram& shader) {
		gl::bindVertexArray(m_vao.id);
		gl::useProgram(shader.id());
	}

public:
	u32 begin = 0, count = 0, instanceCount = 0;
	u32 drawMode = gl::enums::TRIANGLES;

	void draw() {
		gl::drawArraysInstanced(drawMode, begin, count, instanceCount);
	}

private:
	SimpleVAO m_vao;
};
using SDCM = SimpleDrawCallManager;

// since some objects such as tx::RE::RingBufferObject have resize method, it's internal gid might change.
// therefore the set*() functions should be called before every draw call
class DrawCallManager {
public:
	DrawCallManager() = default;

	DrawCallManager(const DrawCallManager&) = delete;
	DrawCallManager& operator=(const DrawCallManager&) = delete;
	DrawCallManager(DrawCallManager&& other) noexcept
	    : m_begin(other.m_begin),
	      m_count(other.m_count),
	      m_instanceCount(other.m_instanceCount),
	      m_vamId(other.m_vamId),
	      m_programId(other.m_programId) {
		other.m_vamId = 0;
		other.m_programId = 0;
	}
	DrawCallManager& operator=(DrawCallManager&& other) noexcept {
		if (this != &other) {
			m_begin = other.m_begin;
			m_count = other.m_count;
			m_instanceCount = other.m_instanceCount;
			m_vamId = other.m_vamId;
			m_programId = other.m_programId;
			other.m_vamId = 0;
			other.m_programId = 0;
		}
		return *this;
	}

	// ================ Draw Call ================

	void draw() {
		preDrawOperations_impl();
		drawArrays_impl(m_begin, m_count);
	}
	void draw(u32 begin, u32 count) {
		preDrawOperations_impl();
		drawArrays_impl(begin, count);
	}

	void drawInstanced() {
		preDrawOperations_impl();
		drawArraysInstanced_impl(m_begin, m_count, m_instanceCount);
	}
	void drawInstanced(u32 begin, u32 count, u32 instanceCount) {
		preDrawOperations_impl();
		drawArraysInstanced_impl(begin, count, instanceCount);
	}

	void drawInstancedOffset() {
		preDrawOperations_impl();
		drawArraysInstancedOffset_impl(m_begin, m_count, m_instanceCount, m_instanceOffset);
	}
	void drawInstancedOffset(u32 begin, u32 count, u32 instanceBegin, u32 instanceCount) {
		preDrawOperations_impl();
		drawArraysInstancedOffset_impl(begin, count, instanceBegin, instanceCount);
	}



	// ================ Configuration ================

	void setBegin(u32 begin) { m_begin = begin; }
	void setCount(u32 count) { m_count = count; }
	void setInstanceCount(u32 instanceCount) { m_instanceCount = instanceCount; }
	void setInstanceOffset(u32 instanceOffset) { m_instanceOffset = instanceOffset; }

	void setParameters(u32 begin, u32 count) {
		m_begin = begin;
		m_count = count;
	}
	void setParameters(u32 begin, u32 count, u32 instanceCount) {
		setParameters(begin, count);
		m_instanceCount = instanceCount;
	}
	void setParameters(u32 begin, u32 count, u32 instanceBegin, u32 instanceCount) {
		setParameters(begin, count, instanceCount);
		m_instanceOffset = instanceBegin;
	}

	// set ShaderProgram
	void setShaders(const ShaderProgram& program) {
		m_programId = program.id();
	}

	// set VertexAttributeManager
	void setVAM(const VAM& in_vam) {
		m_vamId = in_vam.id();
	}

	void setDrawMode(u32 mode) { m_drawMode = mode; }

private:
	void drawArrays_impl(u32 begin, u32 count) const {
		gl::drawArrays(m_drawMode, begin, count);
	}
	void drawArraysInstanced_impl(u32 begin, u32 count, u32 instanceCount) const {
		gl::drawArraysInstanced(m_drawMode, begin, count, instanceCount);
	}
	void drawArraysInstancedOffset_impl(u32 begin, u32 count, u32 instanceBegin, u32 instanceCount) const {
		gl::drawArraysInstancedBaseInstance(m_drawMode, begin, count, instanceCount, instanceBegin);
	}

	// draw call variables

	u32 m_begin = 0, m_count = 0, m_instanceCount = 0, m_instanceOffset = 0;

	gid m_vamId = 0;
	gid m_programId = 0;

	u32 m_drawMode = gl::enums::TRIANGLES;

	inline static thread_local gid currentBindingVAM = 0;
	inline static thread_local gid currentBindingShaderProgram = 0;

	void bindVAM_impl() {
		if (m_vamId && currentBindingVAM != m_vamId) {
			gl::bindVertexArray(m_vamId);
			currentBindingVAM = m_vamId;
		}
	}

	void bindShaders_impl() {
		if (m_programId != currentBindingShaderProgram) {
			currentBindingShaderProgram = m_programId;
			gl::useProgram(m_programId);
		}
	}

	void preDrawOperations_impl() {
		bindVAM_impl();
		bindShaders_impl();
	}
};

// since some objects such as tx::RE::RingBufferObject have resize method, it's internal gid might change.
// therefore the set*() functions should be called before every draw call
class DrawCallManager_legacy {
public:
	DrawCallManager_legacy() = default;

	// Disable Copy
	DrawCallManager_legacy(const DrawCallManager_legacy&) = delete;
	DrawCallManager_legacy& operator=(const DrawCallManager_legacy&) = delete;
	DrawCallManager_legacy(DrawCallManager_legacy&& other) noexcept
	    : m_begin(other.m_begin),
	      m_count(other.m_count),
	      m_instanceCount(other.m_instanceCount),
	      m_vamId(other.m_vamId),
	      m_programId(other.m_programId),
	      m_pipelineId(other.m_pipelineId),
	      m_usePipeline(other.m_usePipeline) {
		other.m_vamId = 0;
		other.m_programId = 0;
		other.m_pipelineId = 0;
	}
	DrawCallManager_legacy& operator=(DrawCallManager_legacy&& other) noexcept {
		if (this != &other) {
			m_begin = other.m_begin;
			m_count = other.m_count;
			m_instanceCount = other.m_instanceCount;
			m_vamId = other.m_vamId;
			m_programId = other.m_programId;
			m_pipelineId = other.m_pipelineId;
			m_usePipeline = other.m_usePipeline;
			other.m_vamId = 0;
			other.m_programId = 0;
			other.m_pipelineId = 0;
		}
		return *this;
	}

	// draw call

	void draw() {
		preDrawOperations_impl();
		drawArrays_impl(m_begin, m_count);
	}
	void draw(u32 begin, u32 count) {
		preDrawOperations_impl();
		drawArrays_impl(begin, count);
	}

	void drawInstanced() {
		preDrawOperations_impl();
		drawArraysInstanced_impl(m_begin, m_count, m_instanceCount);
	}
	void drawInstanced(u32 begin, u32 count, u32 instanceCount) {
		preDrawOperations_impl();
		drawArraysInstanced_impl(begin, count, instanceCount);
	}

	void drawInstancedOffset() {
		preDrawOperations_impl();
		drawArraysInstancedOffset_impl(m_begin, m_count, m_instanceCount, m_instanceOffset);
	}
	void drawInstancedOffset(u32 begin, u32 count, u32 instanceBegin, u32 instanceCount) {
		preDrawOperations_impl();
		drawArraysInstancedOffset_impl(begin, count, instanceBegin, instanceCount);
	}

	// configure

	void setBegin(u32 begin) { m_begin = begin; }
	void setCount(u32 count) { m_count = count; }
	void setInstanceCount(u32 instanceCount) { m_instanceCount = instanceCount; }
	void setInstanceOffset(u32 instanceOffset) { m_instanceOffset = instanceOffset; }


	void setParameters(u32 begin, u32 count) {
		m_begin = begin;
		m_count = count;
	}
	void setParameters(u32 begin, u32 count, u32 instanceCount) {
		setParameters(begin, count);
		m_instanceCount = instanceCount;
	}
	void setParameters(u32 begin, u32 count, u32 instanceBegin, u32 instanceCount) {
		setParameters(begin, count);
		m_instanceOffset = instanceBegin;
		m_instanceCount = instanceCount;
	}

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
	void drawArraysInstancedOffset_impl(u32 begin, u32 count, u32 instanceBegin, u32 instanceCount) const {
		gl::drawArraysInstancedBaseInstance(gl::enums::TRIANGLES, begin, count, instanceCount, instanceBegin);
	}

	// draw call variables

	u32 m_begin = 0, m_count = 0, m_instanceCount = 0, m_instanceOffset = 0;

	gid m_vamId = 0;
	gid m_programId = 0;
	gid m_pipelineId = 0;
	bool m_usePipeline = false;

	inline static thread_local gid currentBindingVAM = 0;
	inline static thread_local gid currentBindingShaderPipeline = 0;
	inline static thread_local gid currentBindingShaderProgram = 0;

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

	void preDrawOperations_impl() {
		bindVAM_impl();
		bindShaders_impl();
	}
};
using DCM = DrawCallManager;
} // namespace RenderEngine
} // namespace tx
