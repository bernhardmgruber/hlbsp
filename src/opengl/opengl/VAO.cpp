#include "VAO.h"

#include <algorithm>

namespace gl {
	VAO::VAO() {
		glGenVertexArrays(1, &m_id);
	}

	VAO::VAO(VAO&& other) {
		swap(other);
	}

	auto VAO::operator=(VAO&& other) -> VAO& {
		swap(other);
		return *this;
	}

	VAO::~VAO() {
		glDeleteVertexArrays(1, &m_id);
	}

	auto VAO::id() const -> GLuint {
		return m_id;
	}

	void VAO::bind() {
		glBindVertexArray(m_id);
	}

	void VAO::swap(VAO& other) {
		using std::swap;
		swap(m_id, other.m_id);
	}
}