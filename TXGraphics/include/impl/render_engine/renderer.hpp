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
		addInstance_impl(sectionIndex, position, textureArr.handle(), static_cast<float>(textureIndex));
	}

	void drawSprites(std::span<const vec2> positions, const TextureArray& textureArr, u32 textureIndex, u32 sectionIndex) {
		u64 textureHandle = textureArr.handle();
		float textureIndexF = static_cast<float>(textureIndex);
		u32 count = positions.size();

		auto positionBuffer = instancePositionBuffer.stage[sectionIndex];
		auto handleBuffer = instanceTextureHandleBuffer.stage[sectionIndex];
		auto indexBuffer = instanceTextureIndexBuffer.stage[sectionIndex];

		u32 oldSize = positionBuffer.size();
		u32 newSize = oldSize + count;

		positionBuffer.resize(newSize);
		handleBuffer.resize(newSize);
		indexBuffer.resize(newSize);

		std::copy(positions.begin(), positions.end(), positionBuffer.begin() + oldSize);
		std::fill(handleBuffer.begin() + oldSize, handleBuffer.end(), textureHandle);
		std::fill(indexBuffer.begin() + oldSize, indexBuffer.end(), textureIndexF);
	}

	// draw call
	void draw() {
		// get metadata
		for (u32 i = 0; i < sectionMetaDatas.size(); i++) {
			if (!getStageBufferSectionMetaData_impl(i))
				throw std::runtime_error(std::string("tx::RE::Renderer: unidentical dynamic buffer section: " + std::to_string(i)));
		}
		// update buffers
		updateDynamicBuffers_impl();
		if (!updateRingBufferOffset())
			throw std::runtime_error("tx::RE::Renderer: Mismatched ring buffer offsets between dynamic buffers.");

		// The VBOs for the ring buffers might be re-allocated (and get a new ID) inside updateDynamicBuffers_impl.
		// We must re-bind them to the VAO to ensure the VAO points to the correct, live buffer objects before drawing.
		VAMSetBuffer(vam, instancePositionBuffer.buffer);
		VAMSetBuffer(vam, instanceTextureHandleBuffer.buffer);
		VAMSetBuffer(vam, instanceTextureIndexBuffer.buffer);

		// draw call
		fm.update();
		for (u32 i = 0; i < sectionMetaDatas.size(); i++) {
			draw_impl(i);
		}
		// clean up
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
		u32 offset, size;
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
	u32 ringBufferOffset = 0;

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

	void addInstance_impl(u32 section, vec2 position, u64 textureHandle, float textureIndex) {
		instancePositionBuffer.stage[section].push_back(position);
		instanceTextureHandleBuffer.stage[section].push_back(textureHandle);
		instanceTextureIndexBuffer.stage[section].push_back(textureIndex);
	}

	// draw call
	// note: all dynamic buffers should have the same length in the same section

	template <class T>
	void updateDynamicBuffer_impl(DBuffer_impl<T>& buffer) {
		u32 size = buffer.stage.dataSize();
		buffer.buffer.bo.push(
		    size, [&](std::span<T> input) {
			    std::copy(buffer.stage.begin(), buffer.stage.end(), input.begin());
		    },
		    FMSubmiter{ fm });
	}
	void updateDynamicBuffers_impl() {
		updateDynamicBuffer_impl(instancePositionBuffer);
		updateDynamicBuffer_impl(instanceTextureHandleBuffer);
		updateDynamicBuffer_impl(instanceTextureIndexBuffer);
	}

	// the actual single draw call
	void draw_impl(u32 sectionIndex) {
		SectionMeta& section = sectionMetaDatas[sectionIndex];
		if (!section.size) return; // skip empty draw calls

		// configure dcm
		dcm.setVAM(vam);
		std::visit(
		    [&](auto& sp) {
			    dcm.setShaders(*sp);
		    },
		    section.shader);

		// draw call
		dcm.drawInstancedOffset(0, 6, section.size, ringBufferOffset + section.offset);
	}

	// security & clean up

	[[nodiscard]] bool getStageBufferSectionMetaData_impl(u32 sectionIndex) {
		sectionMetaDatas[sectionIndex].offset = instancePositionBuffer.stage[sectionIndex].offset();
		sectionMetaDatas[sectionIndex].size = instancePositionBuffer.stage[sectionIndex].size();
		return sectionMetaDatas[sectionIndex].size == instanceTextureHandleBuffer.stage[sectionIndex].size() &&
		       sectionMetaDatas[sectionIndex].size == instanceTextureIndexBuffer.stage[sectionIndex].size() &&
		       sectionMetaDatas[sectionIndex].offset == instanceTextureHandleBuffer.stage[sectionIndex].offset() &&
		       sectionMetaDatas[sectionIndex].offset == instanceTextureIndexBuffer.stage[sectionIndex].offset();
	}
	[[nodiscard]] bool updateRingBufferOffset() {
		ringBufferOffset = instancePositionBuffer.buffer.bo.getNext(FMSubmiter{ fm });
		return ringBufferOffset == instanceTextureHandleBuffer.buffer.bo.getNext(FMSubmiter{ fm }) &&
		       ringBufferOffset == instanceTextureIndexBuffer.buffer.bo.getNext(FMSubmiter{ fm });
	}

	void clearStage_impl() {
		instancePositionBuffer.stage.clearData();
		instanceTextureHandleBuffer.stage.clearData();
		instanceTextureIndexBuffer.stage.clearData();
	}
};

// things to add:
// 1. sprite size
// 2. selectable component -> instancePositionBuffer as base attribute that always exist

} // namespace tx::RenderEngine