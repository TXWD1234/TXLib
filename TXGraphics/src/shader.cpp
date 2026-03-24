// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: shader.cpp

#include "impl/render_engine/shader.hpp"

namespace tx::RenderEngine {

ShaderProgram::ShaderProgram(const VertexShader& vert, const FragmentShader& frag) {
	m_id = gl::createProgram();
	gl::attachShader(m_id, vert.id());
	gl::attachShader(m_id, frag.id());
	gl::linkProgram(m_id);
	gl::getProgramiv(m_id, gl::enums::LINK_STATUS, &m_valid);
}

std::string ShaderProgram::getLog() const {
	int length = 0;
	gl::getProgramiv(m_id, gl::enums::INFO_LOG_LENGTH, &length);
	if (length <= 1) return "";
	std::string str(length - 1, '\0');
	gl::getProgramInfoLog(m_id, length, nullptr, str.data());
	return str;
}

ShaderPipeline::ShaderPipeline(const ShaderProgramComponent<ShaderType::Vertex>& vert, const ShaderProgramComponent<ShaderType::Fragment>& frag) {
	gl::createProgramPipelines(1, &m_id);
	gl::useProgramStages(m_id, gl::enums::VERTEX_SHADER_BIT, vert.id());
	gl::useProgramStages(m_id, gl::enums::FRAGMENT_SHADER_BIT, frag.id());
	// check valid
	gl::validateProgramPipeline(m_id);
	gl::getProgramPipelineiv(m_id, gl::enums::VALIDATE_STATUS, &m_valid);
}

std::string ShaderPipeline::getLog() const {
	int length = 0;
	gl::getProgramPipelineiv(m_id, gl::enums::INFO_LOG_LENGTH, &length);
	if (length <= 1) return "";
	std::string str(length - 1, '\0');
	gl::getProgramPipelineInfoLog(m_id, length, nullptr, str.data());
	return str;
}

} // namespace tx::RenderEngine