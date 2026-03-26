// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: render_engine.hpp

#pragma once
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/buffer.hpp"
#include "impl/gl_core/vertex_attribute_manager.hpp"
#include "impl/gl_core/shader_manager.hpp"
#include "impl/gl_core/draw_call_manager.hpp"
#include "impl/gl_core/texture.hpp"
#include "impl/gl_core/fence_manager.hpp"
#include "impl/gl_core/utility.hpp"
#include "impl/parted_arr.hpp"
#include <unordered_map>


namespace tx::RenderEngine {

class RenderEngine {
public:
	void draw() {
	}



public:
	struct TextureId {
		u32 dimensionId;
		u32 index;
	};

private:
	// class TextureManager_impl {
	// public:
	// 	TextureId addTexture(Coord dimension, std::span<u8> data) {
	// 		u32 dimensionId;
	// 		if (!m_dimensionTable.count(dimension)) {
	// 			// need new TexArr
	// 			dimensionId = m_textures.size();
	// 			addTexture_impl(dimension);
	// 		} else {
	// 			dimensionId = m_dimensionTable.at(dimension);
	// 		}

	// 		return TextureId{
	// 			dimensionId,
	// 			m_textures[dimensionId].push_back(data)
	// 		};
	// 	}

	// 	TextureArray& getTexture(TextureId id) {
	// 		return m_textures[id.dimensionId].ta;
	// 	}

	// private:
	// 	inline static const u32 TextureArraySize = 64;

	// 	struct TexArr_impl {
	// 		TexArr_impl(Coord in_dimension, u32 in_layerCount, TextureRule scaleRule, TextureRule wrapRule)
	// 		    : ta(TextureFormat::RGBA, in_dimension, in_layerCount, 0) {
	// 			ta.setRule(scaleRule, wrapRule);
	// 		}
	// 		TextureArray ta;
	// 		u32 m_size = 0;

	// 		Coord dimension() const { return ta.dimension(); }
	// 		u32 size() const { return m_size; }
	// 		u32 capacity() const { return ta.size(); }

	// 		u32 push_back(std::span<u8> data) {
	// 			if (m_size == capacity()) {
	// 				ta.resize(ta.size() * 2);
	// 			}
	// 			ta.setLayer(m_size, data);
	// 			return m_size++;
	// 		}
	// 	};

	// private:
	// 	std::vector<TexArr_impl> m_textures;
	// 	std::unordered_map<Coord, u32> m_dimensionTable;

	// 	void addTexture_impl(Coord dimension) {
	// 		m_dimensionTable.insert({ dimension, static_cast<u32>(m_textures.size()) });
	// 		m_textures.emplace_back(dimension, TextureArraySize, TextureRule::Linear, TextureRule::Clamp);
	// 	}
	// };

	class BufferManager {
	public:
	private:
		StaticBufferObject<vec2> meshPositionBuffer;
		StaticBufferObject<vec2> meshUVBuffer;
		RingBufferObject<vec2> instancePositionBuffer;
		RingBufferObject<float> instancePositionBufferaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa; // aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
		RingBufferObject<float> instanceTextureIndexBuffer;
	};

private:
	DrawCallManager dcm;
	ShaderManager<SMStyle::Program> sm;
	VertexAttributeManager vam;
	//TextureManager tm;
};
} // namespace tx::RenderEngine