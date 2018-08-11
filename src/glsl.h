#pragma once

#include <GL/glew.h>
#include <GL/GL.h>

#include <experimental/filesystem>

void glslShaderSourceFile(GLuint object, const std::experimental::filesystem::path& filename);
void glslPrintProgramInfoLog(GLuint object);
