// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: texture_manager.hpp

#pragma once
#include "tx/type_traits.hpp"
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/buffer.hpp"
#include "impl/gl_core/texture.hpp"
#include "impl/gl_core/fence_manager.hpp"
#include "impl/gl_core/utility.hpp"
#include <unordered_map>
#include <span>
#include <stdexcept>

namespace tx::RenderEngine {

struct TextureId {
	u32 dimensionId;
	u32 index;
};

template <InstantiationOf<FenceManager> fmT>
class TextureManagerBase {
public:
	TextureId addTexture(Coord dimension, std::span<u8> data) {
		u32 dimensionId;
		if (!m_dimensionTable.count(dimension)) {
			// need new TexArr
			dimensionId = m_textures.size();
			addTexture_impl(dimension);
		} else {
			dimensionId = m_dimensionTable.at(dimension);
		}

		return TextureId{
			dimensionId,
			m_textures[dimensionId].push_back(data, fm)
		};
	}
	TextureId addTexture(u32 dimensionId, std::span<u8> data) {
		if (dimensionId >= m_textures.size())
			throw std::runtime_error("tx::RE::TextureManager::addTexture: Invalid dimensionId.");

		return TextureId{
			dimensionId,
			m_textures[dimensionId].push_back(data, fm)
		};
	}


	TextureArray& getTexture(TextureId id) {
		return m_textures[id.dimensionId].ta;
	}

	void setFenceManager(fmT& in_fm) { fm = &in_fm; }

private:
	inline static const u32 TextureArraySize = 64;
	fmT* fm = nullptr;

	struct TexArr_impl {
		TexArr_impl(Coord in_dimension, u32 in_layerCount, TextureRule scaleRule, TextureRule wrapRule)
		    : ta(TextureFormat::RGBA, in_dimension, in_layerCount, 0, scaleRule, wrapRule) {}
		TextureArray ta;
		u32 m_size = 0;

		Coord dimension() const { return ta.dimension(); }
		u32 size() const { return m_size; }
		u32 capacity() const { return ta.size(); }

		u32 push_back(std::span<u8> data, fmT* fm) {
			if (m_size == capacity()) {
				if (!fm) {
					throw std::runtime_error("tx::RE::TextureManager: FenceManager is not set. Call setFenceManager() before adding textures that may cause a resize.");
				}
				ta.resize(ta.size() * 2, FMAddOperation{ *fm });
			}
			ta.setLayer(m_size, data);
			return m_size++;
		}
	};

private:
	std::vector<TexArr_impl> m_textures;
	std::unordered_map<Coord, u32> m_dimensionTable;

	void addTexture_impl(Coord dimension) {
		m_dimensionTable.insert({ dimension, static_cast<u32>(m_textures.size()) });
		m_textures.emplace_back(dimension, TextureArraySize, TextureRule::Linear, TextureRule::Clamp);
	}
};

using TextureManager = TextureManagerBase<FenceManager_t>;



} // namespace tx::RenderEngine