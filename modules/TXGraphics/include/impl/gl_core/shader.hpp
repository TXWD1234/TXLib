// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXGraphics

#pragma once
#include "impl/gl_core/basic_gl_utils.hpp"
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include "tx/upset.hpp"
#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>

namespace tx {
namespace RenderEngine {

enum class ShaderType : u32 {
	Vertex = gl::enums::VERTEX_SHADER,
	Fragment = gl::enums::FRAGMENT_SHADER,
	Geometry = gl::enums::GEOMETRY_SHADER,
	Compute = gl::enums::COMPUTE_SHADER
};

// ################ Shader Source ################

namespace impl {
template <class T>
concept shader_source = requires(T v) {
	{ v.sources } -> tx::array_type<const char*>;
	{ v.lengths } -> tx::array_type<i32>;
	{ v.size() } -> std::convertible_to<u32>;
};
} // namespace impl

template <size_t N>
struct StaticShaderSource {
	const char* sources[N];
	i32 lengths[N];

	constexpr u32 size() const { return N; }

	constexpr StaticShaderSource(
	    std::convertible_to<std::string_view> auto&&... str)
	    : sources{ std::string_view{ str }.data()... },
	      lengths{ static_cast<i32>(std::string_view{ str }.size())... } {}
};
StaticShaderSource(std::convertible_to<std::string_view> auto... str)
    -> StaticShaderSource<sizeof...(str)>;

// ################ Shader Log Utility ################

namespace impl {
template <class Func>
concept invocable_shader_log_cb =
    std::invocable<Func, bool, std::string_view> ||
    std::invocable<Func, bool, std::string_view, gid>;
template <class Func>
void invokeShaderLogCb(Func&& f, bool a, std::string_view b, gid c) {
	if constexpr (std::invocable<Func, bool, std::string_view>)
		f(a, b);
	else { f(a, b, c); }
}

// template param: true for shader; false for program
template <class GetIVFunc, class GetInfoFunc>
std::string getShaderLog(gid id, GetIVFunc&& ivf, GetInfoFunc&& iff) {
	int length = 0;
	ivf(id, gl::enums::INFO_LOG_LENGTH, &length);
	if (length <= 1) return "";
	std::string str(length - 1, '\0');
	iff(id, length, nullptr, str.data());
	return str;
}
} // namespace impl

// ################ Shader Program ################

template <ShaderType type>
class Shader {
public:
	template <impl::invocable_shader_log_cb Func>
	Shader(impl::shader_source auto str, Func&& f) {
		// DevNote: Assert str.sources.size == str.lengths.size && str.sources.size > 0
		init_impl(str.size(), str.sources, str.lengths, std::forward<Func>(f));
	}
	~Shader() { uninit_impl(); }

	Shader(Shader&& other) noexcept
	    : m_id(other.m_id) { other.m_id = 0; }
	Shader& operator=(Shader&& other) noexcept {
		if (&other != this) {
			uninit_impl();
			m_id = other.m_id;
			other.m_id = 0;
		}
		return *this;
	}
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;


	gid id() const { return m_id; }
	bool valid() const { return m_id; }

private:
	gid m_id = 0;

	template <class Func>
	void init_impl(
	    u32 sourceCount, const char* const source[],
	    const i32* lengths, Func&& f) {
		m_id = gl::createShader(enumval(type));
		gl::shaderSource(m_id, sourceCount, source, lengths);
		gl::compileShader(m_id);

		// check valid
		i32 valid;
		gl::getShaderiv(m_id, gl::enums::COMPILE_STATUS, &valid);

		std::string log = getLog_impl();
		impl::invokeShaderLogCb(std::forward<Func>(f), static_cast<bool>(valid), log, m_id);
		if (!valid) uninit_impl();
	}
	void uninit_impl() {
		if (m_id) gl::deleteShader(m_id);
		m_id = 0;
	}

	std::string getLog_impl() const {
		return impl::getShaderLog(m_id, gl::getShaderiv, gl::getShaderInfoLog);
	}
};

using VertexShader = Shader<ShaderType::Vertex>;
using FragmentShader = Shader<ShaderType::Fragment>;
using GeometryShader = Shader<ShaderType::Geometry>;
using ComputeShader = Shader<ShaderType::Compute>;

class ShaderProgram {
public:
	template <impl::invocable_shader_log_cb Func>
	ShaderProgram(
	    const VertexShader& vert,
	    const FragmentShader& frag, Func&& f) {
		init_impl(vert, frag, std::forward<Func>(f));
	}
	~ShaderProgram() { uninit_impl(); }

	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = delete;
	ShaderProgram(ShaderProgram&& other) noexcept
	    : m_id(other.m_id) { other.m_id = 0; }
	ShaderProgram& operator=(ShaderProgram&& other) noexcept {
		if (this != &other) {
			uninit_impl();
			m_id = other.m_id;
			other.m_id = 0;
		}
		return *this;
	}

	gid id() const { return m_id; }
	bool valid() const { return m_id; }

private:
	gid m_id = 0;

	template <class Func>
	void init_impl(const VertexShader& vert, const FragmentShader& frag, Func&& f) {
		m_id = gl::createProgram();
		gl::attachShader(m_id, vert.id());
		gl::attachShader(m_id, frag.id());
		gl::linkProgram(m_id);
		gl::detachShader(m_id, vert.id());
		gl::detachShader(m_id, frag.id());

		// check valid
		i32 valid;
		gl::getProgramiv(m_id, gl::enums::LINK_STATUS, &valid);

		std::string log = getLog_impl();
		impl::invokeShaderLogCb(std::forward<Func>(f), static_cast<bool>(valid), log, m_id);
		if (!valid) uninit_impl();
	}
	void uninit_impl() {
		if (m_id) gl::deleteProgram(m_id);
		m_id = 0;
	}

	std::string getLog_impl() const {
		return impl::getShaderLog(m_id, gl::getProgramiv, gl::getProgramInfoLog);
	}
};

// ################ Shader Pipeline ################

template <ShaderType ST>
class ShaderProgramComponent {
public:
	template <impl::invocable_shader_log_cb Func>
	ShaderProgramComponent(Shader<ST>&& shader, Func&& f) {
		init_impl(std::move(shader), std::forward<Func>(f));
	}
	~ShaderProgramComponent() { uninit_impl(); }

	ShaderProgramComponent(const ShaderProgramComponent&) = delete;
	ShaderProgramComponent& operator=(const ShaderProgramComponent&) = delete;
	ShaderProgramComponent(ShaderProgramComponent&& other) noexcept
	    : m_id(other.m_id) { other.m_id = 0; }
	ShaderProgramComponent& operator=(ShaderProgramComponent&& other) noexcept {
		if (this != &other) {
			uninit_impl();
			m_id = other.m_id;
			other.m_id = 0;
		}
		return *this;
	}

	gid id() const { return m_id; }
	bool valid() const { return m_id; }

private:
	gid m_id = 0;

	template <class Func>
	void init_impl(Shader<ST> shader, Func&& f) {
		m_id = gl::createProgram();
		gl::programParameteri(m_id, gl::enums::PROGRAM_SEPARABLE, gl::enums::TRUE);
		gl::attachShader(m_id, shader.id());
		gl::linkProgram(m_id);
		gl::detachShader(m_id, shader.id());

		// check valid
		i32 valid;
		gl::getProgramiv(m_id, gl::enums::LINK_STATUS, &valid);

		std::string log = getLog_impl();
		impl::invokeShaderLogCb(std::forward<Func>(f), static_cast<bool>(valid), log, m_id);
		if (!valid) uninit_impl();
	}
	void uninit_impl() {
		if (m_id) gl::deleteProgram(m_id);
		m_id = 0;
	}

	std::string getLog_impl() const {
		return impl::getShaderLog(m_id, gl::getProgramiv, gl::getProgramInfoLog);
	}
};

/**
 * Because the inefficient and inconsistent design of OpenGL's pipeline
 * interface, validation must happen at runtime when VAO, textures are bind.
 * You are just going to accept that and move on. Or simply don't use pipeline.
 * Honestly I regret making this whole thing already.
 */
class ShaderPipeline {
public:
	template <impl::invocable_shader_log_cb Func>
	ShaderPipeline(const ShaderProgramComponent<ShaderType::Vertex>& vert,
	               const ShaderProgramComponent<ShaderType::Fragment>& frag,
	               Func&& f) { init_impl(vert, frag, std::forward<Func>(f)); }
	~ShaderPipeline() { uninit_impl(); }

	ShaderPipeline(const ShaderPipeline&) = delete;
	ShaderPipeline& operator=(const ShaderPipeline&) = delete;
	ShaderPipeline(ShaderPipeline&& other) noexcept
	    : m_id(other.m_id) { other.m_id = 0; }
	ShaderPipeline& operator=(ShaderPipeline&& other) noexcept {
		if (this != &other) {
			uninit_impl();
			m_id = other.m_id;
			other.m_id = 0;
		}
		return *this;
	}

	gid id() const { return m_id; }
	bool valid() const { return m_id; }

private:
	gid m_id = 0;

	template <class Func>
	void init_impl(
	    const ShaderProgramComponent<ShaderType::Vertex>& vert,
	    const ShaderProgramComponent<ShaderType::Fragment>& frag,
	    Func&& f) {
		gl::createProgramPipelines(1, &m_id);
		gl::useProgramStages(m_id, gl::enums::VERTEX_SHADER_BIT, vert.id());
		gl::useProgramStages(m_id, gl::enums::FRAGMENT_SHADER_BIT, frag.id());
		impl::invokeShaderLogCb(std::forward<Func>(f), true, "", m_id);
	}
	void uninit_impl() {
		if (m_id) gl::deleteProgramPipelines(1, &m_id);
		m_id = 0;
	}
};

} // namespace RenderEngine
} // namespace tx
