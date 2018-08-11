#include "glsl.h"

#include <iostream>

#include "IO.h"

void glslShaderSourceFile(GLuint object, const std::experimental::filesystem::path& filename) {
	const auto source = readTextFile(filename);
	const auto p = source.c_str();
	glShaderSource(object, 1, &p, nullptr);
	std::clog << "Read shader from file " << filename << "\n";
}

void glslPrintProgramInfoLog(GLuint object) {
	GLint infologLength = 0;
	glGetProgramiv(object, GL_INFO_LOG_LENGTH, &infologLength);

	if (infologLength > 0) {
		std::string infoLog;
		infoLog.resize(infologLength);
		GLint charsWritten = 0;
		glGetProgramInfoLog(object, infologLength, &charsWritten, infoLog.data());
		if (infoLog[0] != 0 && infoLog != "")
			std::clog << infoLog << "\n";
		else
			std::clog << "(no program info log)\n";
	}
}
