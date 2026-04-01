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

namespace tx::RenderEngine {

constexpr vec2 SquareVertexPositions_CenterOrigin[] = {
	// clang-format off
	{  0.5, -0.5 },
	{ -0.5, -0.5 },
	{  0.5,  0.5 },
	{ -0.5, -0.5 },
	{  0.5,  0.5 },
	{ -0.5,  0.5 }
	// clang-format on
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

	void drawSprite(u32 sectionIndex,
	                vec2 position,
	                u64 textureArrHandle,
	                float textureIndex,
	                vec2 scale = { 1.0f, 1.0f },
	                float rotation = 0.0f,
	                u32 color = 0xFFFFFFFF) {
		addInstance_impl(
		    sectionIndex,
		    position,
		    textureArrHandle,
		    textureIndex,
		    scale,
		    findRotationMat(rotation),
		    color);
	}
	void drawSprite(u32 sectionIndex,
	                vec2 position,
	                u64 textureArrHandle,
	                float textureIndex,
	                vec2 scale = { 1.0f, 1.0f },
	                u32 color = 0xFFFFFFFF) {
		addInstance_impl(
		    sectionIndex,
		    position,
		    textureArrHandle,
		    textureIndex,
		    scale,
		    mat2{ { 1.0f, 0.0f }, { 0.0f, 1.0f } },
		    color);
	}
	// clang-format off

	struct SpriteAtrribs {
		std::span<vec2>  positionBuffer;
		std::span<u64>   textureHandleBuffer;
		std::span<float> textureIndexBuffer;
		std::span<vec2>  scaleBuffer;
		std::span<mat2>  rotationBuffer;
		std::span<u32>   colorBuffer;
	};
	template <std::invocable<SpriteAtrribs&> Func>
	void drawSprites(u32 sectionIndex, u32 count, Func&& editor) {
		auto positionBuffer = instancePositionBuffer     .stage[sectionIndex];
		auto handleBuffer   = instanceTextureHandleBuffer.stage[sectionIndex];
		auto indexBuffer    = instanceTextureIndexBuffer .stage[sectionIndex];
		auto scaleBuffer    = instanceScaleBuffer        .stage[sectionIndex];
		auto rotationBuffer = instanceRotationBuffer     .stage[sectionIndex];
		auto colorBuffer    = instanceColorBuffer        .stage[sectionIndex];

		u32 oldSize = positionBuffer.size();
		u32 newSize = oldSize + count;

		positionBuffer .resize(newSize);
		handleBuffer   .resize(newSize);
		indexBuffer    .resize(newSize);
		scaleBuffer    .resize(newSize);
		rotationBuffer .resize(newSize);
		colorBuffer    .resize(newSize);

		SpriteAtrribs attribs = {
			std::span<vec2> { positionBuffer.begin() + oldSize, positionBuffer.end() },
			std::span<u64>  { handleBuffer  .begin() + oldSize, handleBuffer  .end() },
			std::span<float>{ indexBuffer   .begin() + oldSize, indexBuffer   .end() },
			std::span<vec2> { scaleBuffer   .begin() + oldSize, scaleBuffer   .end() },
			std::span<mat2> { rotationBuffer.begin() + oldSize, rotationBuffer.end() },
			std::span<u32>  { colorBuffer   .begin() + oldSize, colorBuffer   .end() }
		};

		editor(attribs);
	}

	void reserveSprites(u32 sectionIndex, u32 count);
	// clang-format on

	void drawLine(u32 sectionIndex, vec2 start, vec2 end, float thickness = 0.001f, u32 color = 0xFFFFFFFF) {
		vec2 diff = end - start;
		float len = diff.length();
		if (len <= tx::epsilon) return;
		vec2 center = start + diff * 0.5f;
		vec2 nuv = unify(diff, len);
		mat2 rotation = { nuv, leftPerp(nuv) };
		vec2 wh = { len, thickness };

		addInstance_impl(
		    sectionIndex,
		    center,
		    0, 0.0f,
		    wh,
		    rotation,
		    color);
	}





	// draw call
	void draw();






	FenceManager& getFM() { return fm; }

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
	FenceManager fm;

	std::vector<SectionMeta> sectionMetaDatas;
	u32 ringBufferOffset = 0;

	// clang-format off

	BufferHandle<StaticBufferObject<vec2>> meshPositionBuffer;
	BufferHandle<StaticBufferObject<vec2>> meshUVBuffer;
	DBuffer_impl<vec2>  instancePositionBuffer;
	DBuffer_impl<u64>   instanceTextureHandleBuffer;
	DBuffer_impl<float> instanceTextureIndexBuffer;
	DBuffer_impl<vec2>  instanceScaleBuffer;
	DBuffer_impl<mat2>  instanceRotationBuffer;
	DBuffer_impl<u32>   instanceColorBuffer;
	// clang-format on

	void initBuffers_impl();

	// section & metadata
	// mote: each section is a different shader, which means one draw call

	u32 addSection_impl(const ShaderProduct& sp);

	[[nodiscard]] bool getStageBufferSectionMetaData_impl(u32 sectionIndex);
	[[nodiscard]] bool updateRingBufferOffset();

	// OpenGL

	void addInstance_impl(u32 section, vec2 position, u64 textureHandle, float textureIndex, vec2 scale, mat2 rotation, u32 color);

	// update for id changes
	void updateVAM();

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
	void updateDynamicBuffers_impl();

	// the actual single draw call
	void draw_impl(u32 sectionIndex);

	// clean up

	void clearStage_impl();
};

} // namespace tx::RenderEngine