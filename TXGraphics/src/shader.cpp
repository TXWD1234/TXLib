// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: shader.cpp

#include "impl/shader.hpp"

namespace tx::RenderEngine {

ShaderProgram::ShaderProgram(const VertexShader& vert, const FragmentShader& frag) {
	m_id = glCreateProgram();
	glAttachShader(m_id, vert.id());
	glAttachShader(m_id, frag.id());
	glLinkProgram(m_id);
	glGetProgramiv(m_id, GL_LINK_STATUS, &m_valid);
}

std::string ShaderProgram::getLog() const {
	int length = 0;
	glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);
	if (length <= 1) return "";
	std::string str(length - 1, '\0');
	glGetProgramInfoLog(m_id, length, nullptr, str.data());
	return str;
}

ShaderPipeline::ShaderPipeline(const ShaderProgramComponent<ShaderType::Vertex>& vert, const ShaderProgramComponent<ShaderType::Fragment>& frag) {
	glCreateProgramPipelines(1, &m_id);
	glUseProgramStages(m_id, GL_VERTEX_SHADER_BIT, vert.id());
	glUseProgramStages(m_id, GL_FRAGMENT_SHADER_BIT, frag.id());
	// check valid
	glValidateProgramPipeline(m_id);
	glGetProgramPipelineiv(m_id, GL_VALIDATE_STATUS, &m_valid);
}

std::string ShaderPipeline::getLog() const {
	int length = 0;
	glGetProgramPipelineiv(m_id, GL_INFO_LOG_LENGTH, &length);
	if (length <= 1) return "";
	std::string str(length - 1, '\0');
	glGetProgramPipelineInfoLog(m_id, length, nullptr, str.data());
	return str;
}

} // namespace tx::RenderEngine