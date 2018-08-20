#pragma once

#include <initializer_list>
#include <string_view>

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

		auto uniformLocation(std::string_view name) const -> GLint;
		auto attributeLocation(std::string_view name) const -> GLint;

	private:
		void swap(Program& other);

		GLuint m_id = 0;
	};
}