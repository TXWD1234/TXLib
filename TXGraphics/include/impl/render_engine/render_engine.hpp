// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXGraphics

#pragma once
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/shader_manager.hpp"
#include "impl/render_engine/texture_manager.hpp"
#include "impl/gl_core/utility.hpp"

#include "impl/render_engine/renderer.hpp"

namespace tx::RenderEngine {

class RenderEngine {
public:
	// --- Renderer Forwarding ---

	void init() {
		rr.init();
		tm.setFenceManager(rr.getFM());
	}

	// hold id to a section in the renderer, which is corresponding to a pair of shaders
	struct RendererSectionProxy {
		RenderEngine* engine = nullptr;
		u32 id = UINT32_MAX;

		RendererSectionProxy() = default;
		RendererSectionProxy(RenderEngine* e, u32 i) : engine(e), id(i) {}

		void drawSprite(vec2 position, TextureId textureId, vec2 scale = { 1.0f, 1.0f }, float rotation = 0.0f, u32 color = 0xFFFFFFFF) {
			engine->drawSprite(id, position, textureId, scale, rotation, color);
		}
		void drawSprite(vec2 position, u64 textureArrHandle, float textureIndex, vec2 scale = { 1.0f, 1.0f }, float rotation = 0.0f, u32 color = 0xFFFFFFFF) {
			engine->drawSprite(id, position, textureArrHandle, textureIndex, scale, rotation, color);
		}
		template <std::invocable<Renderer::SpriteAtrribs&> Func>
		void drawSprites(u32 count, Func&& editor) {
			engine->drawSprites(id, count, std::forward<Func>(editor));
		}
		void drawSprites(std::span<const vec2> positions, TextureId textureId, vec2 scale = { 1.0f, 1.0f }, float rotation = 0.0f, u32 color = 0xFFFFFFFF) {
			engine->drawSprites(id, positions, textureId, scale, rotation, color);
		}
		void drawLine(vec2 start, vec2 end, float thickness = 0.001f, u32 color = 0xFFFFFFFF) {
			engine->drawLine(id, start, end, thickness, color);
		}

		void reserveSprites(u32 count) { engine->reserveSprites(id, count); }

		// utilities

		void drawLineHorizontal(float y, float thickness = 0.001f, u32 color = 0xFFFFFFFF) {
			engine->drawLine(id, vec2{ -1.0f, y }, vec2{ 1.0f, y }, thickness, color);
		}
		void drawLineVertical(float x, float thickness = 0.001f, u32 color = 0xFFFFFFFF) {
			engine->drawLine(id, vec2{ x, -1.0f }, vec2{ x, 1.0f }, thickness, color);
		}

		bool valid() const { return id != UINT32_MAX && engine; }
	};

	RendererSectionProxy getSectionProxy(u32 sectionIndex) { return RendererSectionProxy(this, sectionIndex); }
	RendererSectionProxy createSectionProxy(Renderer::ShaderProduct sp) { return RendererSectionProxy(this, rr.registerSection(sp)); }
	RendererSectionProxy createSectionProxy(ShaderPipeline& shaderPipeline) { return RendererSectionProxy(this, rr.registerSection(shaderPipeline)); }
	RendererSectionProxy createSectionProxy(ShaderProgram& shaderProgram) { return RendererSectionProxy(this, rr.registerSection(shaderProgram)); }
	RendererSectionProxy createSectionProxy(ShaderPair pid) { return RendererSectionProxy(this, rr.registerSection(&sm.get(pid))); }
	RendererSectionProxy createSectionProxy(ShaderId vertId, ShaderId fragId) { return RendererSectionProxy(this, rr.registerSection(&sm.get(vertId, fragId))); }
	RendererSectionProxy createSectionProxy(const std::string& vertSource, const std::string& fragSource) { return RendererSectionProxy(this, registerSection(vertSource, fragSource)); }

	u32 registerSection(Renderer::ShaderProduct sp) { return rr.registerSection(sp); }
	u32 registerSection(ShaderPipeline& shaderPipeline) { return rr.registerSection(shaderPipeline); }
	u32 registerSection(ShaderProgram& shaderProgram) { return rr.registerSection(shaderProgram); }
	u32 registerSection(ShaderPair pid) { return rr.registerSection(&sm.get(pid)); }
	u32 registerSection(ShaderId vertId, ShaderId fragId) { return rr.registerSection(&sm.get(vertId, fragId)); }

	u32 registerSection(const std::string& vertSource, const std::string& fragSource) {
		ProgramId product;
		if (!addShaderPair(sm, vertSource, fragSource, product)) {
			std::cerr << "[Error]: failed to add shaders.\n";
			return UINT32_MAX;
		}
		return rr.registerSection(sm.get(product));
	}

	void drawSprite(u32 sectionIndex, vec2 position, TextureId textureId, vec2 scale = { 1.0f, 1.0f }, float rotation = 0.0f, u32 color = 0xFFFFFFFF) {
		TextureArray& texArr = tm.getTexture(textureId);
		rr.drawSprite(sectionIndex, position, texArr.handle(), static_cast<float>(textureId.index), scale, rotation, color);
	}
	void drawSprite(u32 sectionIndex, vec2 position, u64 textureArrHandle, float textureIndex, vec2 scale = { 1.0f, 1.0f }, float rotation = 0.0f, u32 color = 0xFFFFFFFF) {
		rr.drawSprite(sectionIndex, position, textureArrHandle, textureIndex, scale, rotation, color);
	}
	template <std::invocable<Renderer::SpriteAtrribs&> Func>
	void drawSprites(u32 sectionIndex, u32 count, Func&& editor) {
		rr.drawSprites(sectionIndex, count, std::forward<Func>(editor));
	}
	// Helper for drawing multiple sprites with the same texture and material properties
	void drawSprites(u32 sectionIndex, std::span<const vec2> positions, TextureId textureId, vec2 scale = { 1.0f, 1.0f }, float rotation = 0.0f, u32 color = 0xFFFFFFFF) {
		TextureArray& texArr = tm.getTexture(textureId);
		u64 handle = texArr.handle();
		float index = static_cast<float>(textureId.index);
		rr.drawSprites(sectionIndex, static_cast<u32>(positions.size()), [&](Renderer::SpriteAtrribs& attribs) {
			std::copy(positions.begin(), positions.end(), attribs.positionBuffer.begin());
			std::fill(attribs.textureHandleBuffer.begin(), attribs.textureHandleBuffer.end(), handle);
			std::fill(attribs.textureIndexBuffer.begin(), attribs.textureIndexBuffer.end(), index);
			std::fill(attribs.scaleBuffer.begin(), attribs.scaleBuffer.end(), scale);
			std::fill(attribs.rotationBuffer.begin(), attribs.rotationBuffer.end(), findRotationMat(rotation));
			std::fill(attribs.colorBuffer.begin(), attribs.colorBuffer.end(), color);
		});
	}
	void drawLine(u32 sectionIndex, vec2 start, vec2 end, float thickness = 0.001f, u32 color = 0xFFFFFFFF) { rr.drawLine(sectionIndex, start, end, thickness, color); }
	void reserveSprites(u32 sectionIndex, u32 count) { rr.reserveSprites(sectionIndex, count); }
	void draw() { rr.draw(); }
	auto& getFM() { return rr.getFM(); }

	// --- ShaderManager Forwarding ---

	template <class... Args>
	ShaderId addVertexShader(Args&&... args) { return sm.addVertexShader(std::forward<Args>(args)...); }
	template <class... Args>
	ShaderId addFragmentShader(Args&&... args) { return sm.addFragmentShader(std::forward<Args>(args)...); }
	ShaderPair linkShaders(ShaderId vertId, ShaderId fragId) { return sm.linkShaders(vertId, fragId); }

	auto& getShader(ShaderPair pid) { return sm.get(pid); }
	auto& getShader(ShaderId vertId, ShaderId fragId) { return sm.get(vertId, fragId); }
	auto& getShaderProgram(ShaderPair pid) { return sm.getShaderProgram(pid); }
	auto& getShaderProgram(ShaderId vertId, ShaderId fragId) { return sm.getShaderProgram(vertId, fragId); }
	auto& getShaderPipeline(ShaderPair pid) { return sm.getShaderPipeline(pid); }
	auto& getShaderPipeline(ShaderId vertId, ShaderId fragId) { return sm.getShaderPipeline(vertId, fragId); }

	bool compileSucceed(ShaderId sid) { return sm.compileSucceed(sid); }
	std::string getCompileLog(ShaderId sid) { return sm.getCompileLog(sid); }
	bool linkSucceed(ShaderPair pid) { return sm.linkSucceed(pid); }
	std::string getLinkLog(ShaderPair pid) { return sm.getLinkLog(pid); }

	// --- TextureManager Forwarding ---

	TextureId addTexture(Coord dimension, std::span<u8> data) { return tm.addTexture(dimension, data); }
	TextureId addTexture(u32 dimensionId, std::span<u8> data) { return tm.addTexture(dimensionId, data); }
	TextureArray& getTexture(TextureId id) { return tm.getTexture(id); }
	Coord getTextureDimension(u32 dimensionId) const { return tm.getDimension(dimensionId); }

private:
	Renderer rr;
	ShaderManager<SMStyle::Program> sm;
	TextureManager tm;
};
using RE = RenderEngine;
// hold id to a section in the renderer, which is corresponding to a pair of shaders
using RSP = RenderEngine::RendererSectionProxy;
} // namespace tx::RenderEngine