#pragma once

#include <GL/glew.h>

namespace gl {
	class Buffer {
	public:
		Buffer();
		Buffer(const Buffer&) = delete;
		auto operator=(const Buffer&) -> Buffer& = delete;
		Buffer(Buffer&& other);
		auto operator=(Buffer&& other) -> Buffer&;
		~Buffer();

		auto id() const -> GLuint;
		void bind(GLenum target);

	private:
		void swap(Buffer& other);

		GLuint m_id = 0;
	};
}