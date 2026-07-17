// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXGraphics

#pragma once
#include "impl/render_engine/basic_gl_utils.hpp"
#include "impl/render_engine/buffer.hpp"
#include "impl/render_engine/fence.hpp"
#include <tuple>

namespace tx {
namespace RenderEngine {

template <class T, BufferType iom>
struct BufferId {
	using type = T;
	u32 index;
};


class BufferManager {
	template <class T>
	using SBufs_t = std::vector<StaticBufferObject<T>>; // static buffer array
	template <class T>
	using DBufs_t = std::vector<RingBufferObject<T>>; // dynamic buffer array

	using BufferStorage_t =
	    std::tuple<
	        SBufs_t<float>,
	        SBufs_t<int>,
	        SBufs_t<u8>,
	        SBufs_t<u32>,
	        SBufs_t<vec2>,
	        SBufs_t<Coord>,
	        DBufs_t<float>,
	        DBufs_t<int>,
	        DBufs_t<u8>,
	        DBufs_t<u32>,
	        DBufs_t<vec2>,
	        DBufs_t<Coord>>;

public:
	class Initializer;
	BufferManager() = default;
	template <class InitFunc>
	BufferManager(InitFunc f) {
		static_assert(std::is_invocable_v<InitFunc, Initializer&>, "tx::RE::BufferManager: InitFunc must take Initializer&");
		Initializer initer;
		f(initer);
		init_impl(std::move(initer));
	}
	BufferManager(Initializer&& initer) {
		init_impl(std::move(initer));
	}
	~BufferManager() = default;

	// Disable Copy
	BufferManager(const BufferManager&) = delete;
	BufferManager& operator=(const BufferManager&) = delete;
	BufferManager(BufferManager&&) noexcept = default;
	BufferManager& operator=(BufferManager&&) noexcept = default;


	class Initializer {
		friend class BufferManager;

	public:
		Initializer() = default;
		~Initializer() = default;

		// Disable Copy
		Initializer(const Initializer&) = delete;
		Initializer& operator=(const Initializer&) = delete;

		// Move Semantics
		Initializer(Initializer&&) noexcept = default;
		Initializer& operator=(Initializer&&) noexcept = default;

		template <class T, class... Args>
		BufferId<T, BufferType::Static> addStatic(Args&&... args) {
			SBufs_t<T>& bufferArray = std::get<SBufs_t<T>>(bufferStorage);
			bufferArray.emplace_back(std::forward<Args>(args)...);
			return { static_cast<u32>(bufferArray.size() - 1) };
		}
		template <class T, class... Args>
		BufferId<T, BufferType::Dynamic> addDynamic(Args&&... args) {
			DBufs_t<T>& bufferArray = std::get<DBufs_t<T>>(bufferStorage);
			bufferArray.emplace_back(std::forward<Args>(args)...);
			return { static_cast<u32>(bufferArray.size() - 1) };
		}

	private:
		BufferStorage_t bufferStorage;
	};

	template <class T>
	std::vector<T>& getHandle(BufferId<T, BufferType::Dynamic> id) {
		return getDBufArr_impl<T>()[id.index].getStagingBuffer();
	}
	template <class T, BufferType iom>
	gid getId(BufferId<T, iom> id) {
		if constexpr (iom == BufferType::Dynamic) {
			return getDBufArr_impl<T>()[id.index].id();
		} else {
			return getSBufArr_impl<T>()[id.index].id();
		}
	}

private:
	BufferStorage_t bufferStorage;

	void init_impl(Initializer&& initer) {
		bufferStorage = std::move(initer.bufferStorage);
	}

	template <class T>
	DBufs_t<T>& getDBufArr_impl() {
		return std::get<DBufs_t<T>>(bufferStorage);
	}
	template <class T>
	SBufs_t<T>& getSBufArr_impl() {
		return std::get<SBufs_t<T>>(bufferStorage);
	}
};
using BM = BufferManager;
using BMIniter = BM::Initializer;












} // namespace RenderEngine
}; // namespace tx