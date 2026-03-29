// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: animation.hpp

#include <vector>
#include <span>
#include "tx/math.h"
#include "impl/render_engine/texture_manager.hpp"

namespace tx {

class Animation {
	using id = RenderEngine::TextureId;

public:
	Animation(u32 frameRate = 1) : m_frameSubCounterMax(frameRate) {}
	Animation(std::span<id> frames, u32 frameRate = 1)
	    : m_frames(frames.begin(), frames.end()), m_frameSubCounterMax(frameRate) {}

	void addFrame(id id) { m_frames.push_back(id); }

	template <class... Args>
	auto insertFrame(Args&&... args) { return m_frames.insert(std::forward<Args>(args)...); }

	auto begin() { return m_frames.begin(); }
	auto end() { return m_frames.end(); }
	auto begin() const { return m_frames.begin(); }
	auto end() const { return m_frames.end(); }

	u32 size() const { return m_frames.size(); }

	id& operator[](u32 index) { return m_frames[index]; }
	const id& operator[](u32 index) const { return m_frames[index]; }

	const id& next() {
		// Get the texture for the current frame BEFORE advancing the counters.
		const id& current_id = m_frames[m_currentFrame];

		// Now, advance the counters for the NEXT time next() is called.
		m_frameSubCounter++;
		if (m_frameSubCounter >= m_frameSubCounterMax) {
			m_frameSubCounter = 0;
			m_currentFrame++;
			if (m_currentFrame >= m_frames.size()) {
				m_currentFrame = 0;
			}
		}
		return current_id;
	}

private:
	std::vector<id> m_frames;
	u32 m_currentFrame = 0, m_frameSubCounter = 0, m_frameSubCounterMax = 0;
};

} // namespace tx