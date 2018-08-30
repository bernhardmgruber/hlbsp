#pragma once

#include <gl/glew.h>

namespace gl {
	class VAO {
	public:
		VAO();
		VAO(const VAO&) = delete;
		VAO& operator=(const VAO&) = delete;
		VAO(VAO&& other);
		VAO& operator=(VAO&& other);
		~VAO();

		auto id() const -> GLuint;
		void bind();

	private:
		void swap(VAO& other);

		GLuint m_id = 0;
	};
}