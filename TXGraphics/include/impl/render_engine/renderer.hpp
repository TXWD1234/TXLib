// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: renderer.hpp

#pragma once
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/draw_call_manager.hpp"
#include "impl/gl_core/buffer.hpp"
#include "impl/gl_core/vertex_attribute_manager.hpp"
#include "impl/gl_core/shader.hpp"
#include "impl/gl_core/utility.hpp"
#include "impl/render_engine/texture_manager.hpp"
#include "impl/parted_arr.hpp"

#include <variant>
#include <unordered_map>

namespace tx::RenderEngine {

constexpr vec2 SquareVertexPositions_CenterOrigin[] = {
	{ 0.5, -0.5 },
	{ -0.5, -0.5 },
	{ 0.5, 0.5 },
	{ -0.5, -0.5 },
	{ 0.5, 0.5 },
	{ -0.5, 0.5 }
};
constexpr vec2 SquareVertexUVs_CenterOrigin[] = {
	{ 1.0, 0.0 },
	{ 0.0, 0.0 },
	{ 1.0, 1.0 },
	{ 0.0, 0.0 },
	{ 1.0, 1.0 },
	{ 0.0, 1.0 }
};

class Renderer {
public:
	Renderer() = default;
	void init() {
		initBuffers_impl();
	}

	using ShaderProduct = std::variant<
	    ShaderProgram*,
	    ShaderPipeline*>;
	using SectionId = u32;
	struct ShaderProductHash {
		size_t operator()(const ShaderProduct& sp) const {
			return std::visit(
			    [](auto* obj) -> size_t {
				    return std::hash<const void*>{}(obj);
			    },
			    sp);
		}
	};

	u32 registerSection(ShaderProduct sp) {
		return addSection_impl(sp);
	}
	u32 registerSection(ShaderPipeline& shaderPipeline) { return registerSection(&shaderPipeline); }
	u32 registerSection(ShaderProgram& shaderProgram) { return registerSection(&shaderProgram); }


	// per frame draw operations

	void drawSprite(vec2 position, const TextureArray& textureArr, u32 textureIndex, u32 sectionIndex) {
		addInstance_impl(sectionIndex, position, textureArr.handle(), textureIndex);
	}

	// draw call
	void draw() {
		for (u32 i = 0; i < sectionMetaDatas.size(); i++) {
			draw_impl(i);
		}
		clearStage_impl();
	}





	FenceManager_t& getFM() { return fm; }

private:
	template <class T>
	struct DBuffer_impl {
		PartedArr<T> stage;
		BufferHandle<RingBufferObject<T>> buffer;
	};

	// sections are separated by different shaders
	struct SectionMeta {
		ShaderProduct shader;
	};

private:
	DrawCallManager dcm;
	VertexAttributeManager vam;
	FenceManager_t fm;

	BufferHandle<StaticBufferObject<vec2>> meshPositionBuffer;
	BufferHandle<StaticBufferObject<vec2>> meshUVBuffer;
	DBuffer_impl<vec2> instancePositionBuffer;
	DBuffer_impl<u64> instanceTextureHandleBuffer;
	DBuffer_impl<float> instanceTextureIndexBuffer;

	std::vector<SectionMeta> sectionMetaDatas;

	void initBuffers_impl() {
		vam = VertexAttributeManager([&](VAMIniter& initer) {
			meshPositionBuffer.id = initer.addAttrib<vec2>();
			meshUVBuffer.id = initer.addAttrib<vec2>();
			instancePositionBuffer.buffer.id = initer.addAttribInstanced<vec2>();
			instanceTextureHandleBuffer.buffer.id = initer.addAttribInstanced<u64>();
			instanceTextureIndexBuffer.buffer.id = initer.addAttribInstanced<float>();
		});
		meshPositionBuffer.bo.alloc(6, SquareVertexPositions_CenterOrigin);
		meshUVBuffer.bo.alloc(6, SquareVertexUVs_CenterOrigin);
		instancePositionBuffer.buffer.bo.alloc();
		instanceTextureHandleBuffer.buffer.bo.alloc();
		instanceTextureIndexBuffer.buffer.bo.alloc();
		VAMSetBuffer(vam, meshPositionBuffer);
		VAMSetBuffer(vam, meshUVBuffer);
		VAMSetBuffer(vam, instancePositionBuffer.buffer);
		VAMSetBuffer(vam, instanceTextureHandleBuffer.buffer);
		VAMSetBuffer(vam, instanceTextureIndexBuffer.buffer);
	}

	// section
	// mote: each section is a different shader, which means one draw call

	u32 addSection_impl(const ShaderProduct& sp) {
		instancePositionBuffer.stage.addPartition();
		instanceTextureHandleBuffer.stage.addPartition();
		instanceTextureIndexBuffer.stage.addPartition();
		sectionMetaDatas.push_back({ sp });
		return sectionMetaDatas.size() - 1;
	}

	// OpenGL

	void addInstance_impl(u32 section, vec2 position, u64 textureHandle, u32 textureIndex) {
		instancePositionBuffer.stage[section].push_back(position);
		instanceTextureHandleBuffer.stage[section].push_back(textureHandle);
		instanceTextureIndexBuffer.stage[section].push_back(static_cast<float>(textureIndex));
	}

	// draw call
	// note: all dynamic buffers should have the same length in the current stage

	template <class T>
	void updateDynamicBuffer_impl(u32 sectionIndex, DBuffer_impl<T>& buffer) {
		auto stageBuffer = buffer.stage[sectionIndex];
		u32 stageSize = stageBuffer.size();
		buffer.buffer.bo.push(
		    stageSize, [&](std::span<T> input) {
			    std::copy(stageBuffer.begin(), stageBuffer.end(), input.begin());
		    },
		    FMSubmiter{ fm });
		VAMUpdateRingBuffer(vam, buffer.buffer, FMSubmiter{ fm });
	}
	u32 updateDynamicBuffers_impl(u32 sectionIndex) {
		if (!checkStageBufferSizeIdentical_impl(sectionIndex))
			throw std::runtime_error(std::string("tx::RE::Renderer: unidentical dynamic buffer section: " + std::to_string(sectionIndex)));
		updateDynamicBuffer_impl(sectionIndex, instancePositionBuffer);
		updateDynamicBuffer_impl(sectionIndex, instanceTextureHandleBuffer);
		updateDynamicBuffer_impl(sectionIndex, instanceTextureIndexBuffer);
		return instancePositionBuffer.stage[sectionIndex].size();
	}

	// the actual single draw call
	void draw_impl(u32 sectionIndex) {
		// update buffer
		u32 count = updateDynamicBuffers_impl(sectionIndex);

		// configure dcm
		dcm.setVAM(vam);
		std::visit(
		    [&](auto& sp) {
			    dcm.setShaders(*sp);
		    },
		    sectionMetaDatas[sectionIndex].shader);

		// draw call
		fm.update();
		dcm.drawInstanced(0, 6, count);
	}

	// security & clean up

	bool checkStageBufferSizeIdentical_impl(u32 sectionIndex) const {
		const u32 expectedSize = instancePositionBuffer.stage[sectionIndex].size();
		return expectedSize == instanceTextureHandleBuffer.stage[sectionIndex].size() &&
		       expectedSize == instanceTextureIndexBuffer.stage[sectionIndex].size();
	}

	void clearStage_impl() {
		instancePositionBuffer.stage.clearData();
		instanceTextureHandleBuffer.stage.clearData();
		instanceTextureIndexBuffer.stage.clearData();
	}
};

// things to add:
// 1. sprite size
// 2. resolve shader registery problem

} // namespace tx::RenderEngine