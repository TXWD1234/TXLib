// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXGraphics

#include "impl/render_engine/renderer.hpp"

namespace tx::RenderEngine {

// clang-format off

void Renderer::reserveSprites(u32 sectionIndex, u32 count) {
	auto positionBuffer = instancePositionBuffer     .stage[sectionIndex];
	auto handleBuffer   = instanceTextureHandleBuffer.stage[sectionIndex];
	auto indexBuffer    = instanceTextureIndexBuffer .stage[sectionIndex];
	auto scaleBuffer    = instanceScaleBuffer        .stage[sectionIndex];
	auto rotationBuffer = instanceRotationBuffer     .stage[sectionIndex];
	auto colorBuffer    = instanceColorBuffer        .stage[sectionIndex];
	u32 targetSize = positionBuffer.size() + count;
	positionBuffer.reserve(targetSize);
	handleBuffer  .reserve(targetSize);
	indexBuffer   .reserve(targetSize);
	scaleBuffer   .reserve(targetSize);
	rotationBuffer.reserve(targetSize);
	colorBuffer   .reserve(targetSize);
}

void Renderer::draw() {
	// get metadata
	for (u32 i = 0; i < sectionMetaDatas.size(); i++) {
		if (!getStageBufferSectionMetaData_impl(i))
			throw std::runtime_error(std::string("tx::RE::Renderer: unidentical dynamic buffer section: " + std::to_string(i)));
	}
	// update buffers
	updateDynamicBuffers_impl();
	if (!updateRingBufferOffset())
		throw std::runtime_error("tx::RE::Renderer: Mismatched ring buffer offsets between dynamic buffers.");
	updateVAM();

	// draw call
	fm.update();
	for (u32 i = 0; i < sectionMetaDatas.size(); i++) {
		draw_impl(i);
	}
	// clean up
	clearStage_impl();
}

void Renderer::initBuffers_impl() {
	vam = VertexAttributeManager([&](VAMIniter& initer) {
		meshPositionBuffer.id                 = initer.addAttrib         <vec2> ();
		meshUVBuffer.id                       = initer.addAttrib         <vec2> ();
		instancePositionBuffer     .buffer.id = initer.addAttribInstanced<vec2> ();
		instanceTextureHandleBuffer.buffer.id = initer.addAttribInstanced<u64>  ();
		instanceTextureIndexBuffer .buffer.id = initer.addAttribInstanced<float>();
		instanceScaleBuffer        .buffer.id = initer.addAttribInstanced<vec2> ();
		instanceRotationBuffer     .buffer.id = initer.addAttribInstanced<mat2> ();
		instanceColorBuffer        .buffer.id = initer.addAttribInstanced<u32>  ();
	});
	meshPositionBuffer.bo.alloc(6, SquareVertexPositions_CenterOrigin);
	meshUVBuffer      .bo.alloc(6, SquareVertexUVs_CenterOrigin);
	instancePositionBuffer     .buffer.bo.alloc();
	instanceTextureHandleBuffer.buffer.bo.alloc();
	instanceTextureIndexBuffer .buffer.bo.alloc();
	instanceScaleBuffer        .buffer.bo.alloc();
	instanceRotationBuffer     .buffer.bo.alloc();
	instanceColorBuffer        .buffer.bo.alloc();
	VAMSetBuffer(vam, meshPositionBuffer);
	VAMSetBuffer(vam, meshUVBuffer);
	VAMSetBuffer(vam, instancePositionBuffer     .buffer);
	VAMSetBuffer(vam, instanceTextureHandleBuffer.buffer);
	VAMSetBuffer(vam, instanceTextureIndexBuffer .buffer);
	VAMSetBuffer(vam, instanceScaleBuffer        .buffer);
	VAMSetBuffer(vam, instanceRotationBuffer     .buffer);
	VAMSetBuffer(vam, instanceColorBuffer        .buffer);
}

u32 Renderer::addSection_impl(const ShaderProduct& sp) {
	instancePositionBuffer     .stage.addPartition();
	instanceTextureHandleBuffer.stage.addPartition();
	instanceTextureIndexBuffer .stage.addPartition();
	instanceScaleBuffer        .stage.addPartition();
	instanceRotationBuffer     .stage.addPartition();
	instanceColorBuffer        .stage.addPartition();
	sectionMetaDatas.push_back({ sp });
	return sectionMetaDatas.size() - 1;
}

bool Renderer::getStageBufferSectionMetaData_impl(u32 sectionIndex) {
	sectionMetaDatas[sectionIndex].offset = instancePositionBuffer.stage[sectionIndex].offset();
	sectionMetaDatas[sectionIndex].size   = instancePositionBuffer.stage[sectionIndex].size();
	return sectionMetaDatas[sectionIndex].size   == instanceTextureHandleBuffer.stage[sectionIndex].size()   &&
	       sectionMetaDatas[sectionIndex].size   == instanceTextureIndexBuffer .stage[sectionIndex].size()   &&
	       sectionMetaDatas[sectionIndex].size   == instanceScaleBuffer        .stage[sectionIndex].size()   &&
	       sectionMetaDatas[sectionIndex].size   == instanceRotationBuffer     .stage[sectionIndex].size()   &&
	       sectionMetaDatas[sectionIndex].size   == instanceColorBuffer        .stage[sectionIndex].size()   &&
	       sectionMetaDatas[sectionIndex].offset == instanceTextureHandleBuffer.stage[sectionIndex].offset() &&
	       sectionMetaDatas[sectionIndex].offset == instanceTextureIndexBuffer .stage[sectionIndex].offset() &&
	       sectionMetaDatas[sectionIndex].offset == instanceScaleBuffer        .stage[sectionIndex].offset() &&
	       sectionMetaDatas[sectionIndex].offset == instanceRotationBuffer     .stage[sectionIndex].offset() &&
	       sectionMetaDatas[sectionIndex].offset == instanceColorBuffer        .stage[sectionIndex].offset();
}

bool Renderer::updateRingBufferOffset() {
	ringBufferOffset = instancePositionBuffer.buffer.bo.getNext(FMSubmiter{ fm });
	return ringBufferOffset == instanceTextureHandleBuffer.buffer.bo.getNext(FMSubmiter{ fm }) &&
	       ringBufferOffset == instanceTextureIndexBuffer .buffer.bo.getNext(FMSubmiter{ fm }) &&
	       ringBufferOffset == instanceScaleBuffer        .buffer.bo.getNext(FMSubmiter{ fm }) &&
	       ringBufferOffset == instanceRotationBuffer     .buffer.bo.getNext(FMSubmiter{ fm }) &&
	       ringBufferOffset == instanceColorBuffer        .buffer.bo.getNext(FMSubmiter{ fm });
}

void Renderer::addInstance_impl(u32 section, vec2 position, u64 textureHandle, float textureIndex, vec2 scale, mat2 rotation, u32 color) {
	instancePositionBuffer     .stage[section].push_back(position);
	instanceTextureHandleBuffer.stage[section].push_back(textureHandle);
	instanceTextureIndexBuffer .stage[section].push_back(textureIndex);
	instanceScaleBuffer        .stage[section].push_back(scale);
	instanceRotationBuffer     .stage[section].push_back(rotation);
	instanceColorBuffer        .stage[section].push_back(color);
}

void Renderer::updateVAM() {
	// The VBOs for the ring buffers might be re-allocated (and get a new ID) inside updateDynamicBuffers_impl.
	// We must re-bind them to the VAO to ensure the VAO points to the correct, live buffer objects before drawing.
	VAMSetBuffer(vam, instancePositionBuffer     .buffer);
	VAMSetBuffer(vam, instanceTextureHandleBuffer.buffer);
	VAMSetBuffer(vam, instanceTextureIndexBuffer .buffer);
	VAMSetBuffer(vam, instanceScaleBuffer        .buffer);
	VAMSetBuffer(vam, instanceRotationBuffer     .buffer);
	VAMSetBuffer(vam, instanceColorBuffer        .buffer);
}

void Renderer::updateDynamicBuffers_impl() {
	updateDynamicBuffer_impl(instancePositionBuffer     );
	updateDynamicBuffer_impl(instanceTextureHandleBuffer);
	updateDynamicBuffer_impl(instanceTextureIndexBuffer );
	updateDynamicBuffer_impl(instanceScaleBuffer        );
	updateDynamicBuffer_impl(instanceRotationBuffer     );
	updateDynamicBuffer_impl(instanceColorBuffer        );
}

void Renderer::draw_impl(u32 sectionIndex) {
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
	dcm.drawInstancedOffset(0, 6, ringBufferOffset + section.offset, section.size);
}

void Renderer::clearStage_impl() {
	instancePositionBuffer     .stage.clearData();
	instanceTextureHandleBuffer.stage.clearData();
	instanceTextureIndexBuffer .stage.clearData();
	instanceScaleBuffer        .stage.clearData();
	instanceRotationBuffer     .stage.clearData();
	instanceColorBuffer        .stage.clearData();
}

// clang-format on
} // namespace tx::RenderEngine
