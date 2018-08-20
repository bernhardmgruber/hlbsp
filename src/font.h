#pragma once

#include <GL/glew.h>
#include <GL/gl.h>

#include <string>

class Font {
public:
	Font() = default;
	Font(const std::string& name, int height);
	Font(const Font&) = delete;
	Font& operator=(const Font&) = delete;
	Font(Font&& other);
	Font& operator=(Font&& other);
	~Font();

	friend void glPuts(int nX, int nY, const Font& font, const std::string& text);

private:
	void swap(Font& other);

	GLuint m_id = 0;
};
