#pragma once

#include <initializer_list>
#include <string>
#include <unordered_map>

#include "Shader.h"

namespace gl {
	class Program {
	public:
		Program() = default;
		Program(std::initializer_list<Shader> shaders);
		Program(const Program&) = delete;
		Program& operator=(const Program&) = delete;
		Program(Program&& other);
		Program& operator=(Program&& other);
		~Program();

		auto id() const -> GLuint;
		void use() const;

		auto attributeLocation(const std::string& name) const -> GLint;
		auto uniformLocation(const std::string& name) const -> GLint;
		auto uniformBlockIndex(const std::string& name) const -> GLint;

	private:
		void swap(Program& other);

		GLuint m_id = 0;
		std::unordered_map<std::string, GLint> m_attributes;
		std::unordered_map<std::string, GLint> m_uniforms;
		std::unordered_map<std::string, GLint> m_uniformBlocks;
	};
}