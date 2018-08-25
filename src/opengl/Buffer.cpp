#include "Buffer.h"

#include <algorithm>

namespace gl {
	Buffer::Buffer() {
		glGenBuffers(1, &m_id);
	}

	Buffer::Buffer(Buffer&& other) {
		swap(other);
	}

	Buffer& Buffer::operator=(Buffer&& other) {
		swap(other);
		return *this;
	}

	Buffer::~Buffer() {
		glDeleteBuffers(1, &m_id);
	}

	auto Buffer::id() const -> GLuint {
		return m_id;
	}

	void Buffer::bind(GLenum target) {
		glBindBuffer(target, m_id);
	}

	void Buffer::swap(Buffer& other) {
		using std::swap;
		swap(m_id, other.m_id);
	}
}