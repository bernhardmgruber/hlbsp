#include "Program.h"

#include <iostream>

#include "../../IO.h"

namespace gl {
	namespace {
		template<typename GetFunc, typename GetLocationFunc>
		auto loadAttribUniformMap(GLuint program, GLenum countFlag, GetFunc getFunc, GetLocationFunc getLocationFunc) {
			std::unordered_map<std::string, GLint> map;

			GLint count;
			glGetProgramiv(program, countFlag, &count);
			for (auto i = 0; i < count; i++) {
				GLint length;
				std::string name(255, '\0');
				if constexpr (std::is_invocable_v<GetFunc, GLuint, GLuint, GLsizei, GLsizei*, GLchar*>)
					getFunc(program, i, static_cast<GLsizei>(name.size()), &length, name.data());
				else {
					GLsizei size;
					GLenum type;
					getFunc(program, i, static_cast<GLsizei>(name.size()), &length, &size, &type, name.data());
				}

				name.resize(length);
				if (const auto loc = getLocationFunc(program, name.c_str()); loc != -1)
					map.emplace(std::move(name), loc);
			}

			return map;
		}
	}

	Program::Program(std::initializer_list<Shader> shaders) {
		m_id = glCreateProgram();
		for (const auto& shader : shaders)
			glAttachShader(m_id, shader.id());
		glLinkProgram(m_id);

		GLint status;
		glGetProgramiv(m_id, GL_LINK_STATUS, &status);
		GLint length;
		glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);
		std::string buildLog;
		buildLog.resize(length);
		glGetProgramInfoLog(m_id, length, nullptr, buildLog.data());

		if (status != GL_TRUE)
			throw std::runtime_error("Failed to link program:\n" + buildLog);
		else
			std::clog << "Program link log:\n"
					  << buildLog << "\n";

		m_attributes = loadAttribUniformMap(m_id, GL_ACTIVE_ATTRIBUTES, glGetActiveAttrib, glGetAttribLocation);
		m_uniforms = loadAttribUniformMap(m_id, GL_ACTIVE_UNIFORMS, glGetActiveUniform, glGetUniformLocation);
		m_uniformBlocks = loadAttribUniformMap(m_id, GL_ACTIVE_UNIFORM_BLOCKS, glGetActiveUniformBlockName, glGetUniformBlockIndex);
	}

	Program::Program(Program&& other) {
		swap(other);
	}

	Program& Program::operator=(Program&& other) {
		swap(other);
		return *this;
	}

	Program::~Program() {
		if (m_id != 0)
			glDeleteProgram(m_id);
	}

	auto Program::id() const -> GLuint {
		return m_id;
	}

	void Program::use() const {
		glUseProgram(m_id);
	}

	auto Program::uniformLocation(const std::string& name) const -> GLint {
		return m_uniforms.at(name);
	}

	auto Program::uniformBlockIndex(const std::string& name) const -> GLint {
		return m_uniformBlocks.at(name);
	}

	auto Program::attributeLocation(const std::string& name) const -> GLint {
		return m_attributes.at(name);
	}

	void Program::swap(Program& other) {
		using std::swap;
		swap(m_id, other.m_id);
		swap(m_attributes, other.m_attributes);
		swap(m_uniforms, other.m_uniforms);
	}
}