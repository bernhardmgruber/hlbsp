#include "glsl.h"

#include <iostream>

#include "IO.h"

void glslShaderSourceFile(GLuint object, const std::experimental::filesystem::path& filename) {
	const auto source = readTextFile(filename);
	const auto p = source.c_str();
	glShaderSource(object, 1, &p, nullptr);
	std::clog << "Read shader from file " << filename << "\n";
}

namespace {
	template <typename GetIVFunc, typename GetLogFunc>
	void glslPrintInfoLog(GLuint object, GetIVFunc getIV, GetLogFunc getLog) {
		GLint infologLength = 0;
		getIV(object, GL_INFO_LOG_LENGTH, &infologLength);

		if (infologLength > 0) {
			std::string infoLog;
			infoLog.resize(infologLength);
			GLint charsWritten = 0;
			getLog(object, infologLength, &charsWritten, infoLog.data());
			if (infoLog[0] != 0 && infoLog != "")
				std::clog << infoLog << "\n";
			else
				std::clog << "(no info log)\n";
		}
	}
}

void glslPrintShaderInfoLog(GLuint object) {
	glslPrintInfoLog(object, glGetShaderiv, glGetShaderInfoLog);
}

void glslPrintProgramInfoLog(GLuint object) {
	glslPrintInfoLog(object, glGetProgramiv, glGetProgramInfoLog);
}
