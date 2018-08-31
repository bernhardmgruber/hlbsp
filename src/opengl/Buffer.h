#pragma once

#include <GL/glew.h>

namespace gl {
	class Buffer {
	public:
		Buffer();
		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;
		Buffer(Buffer&& other);
		Buffer& operator=(Buffer&& other);
		~Buffer();

		auto id() const -> GLuint;
		void bind(GLenum target);

	private:
		void swap(Buffer& other);

		GLuint m_id = 0;
	};
}