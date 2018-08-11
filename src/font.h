#pragma once

#include <GL/glew.h>
#include <GL/gl.h>

#include <string>

GLuint createFont(const char* name, int height);
void   deleteFont(GLuint font);

void glPuts(int nX, int nY, GLuint font, const std::string& text);
