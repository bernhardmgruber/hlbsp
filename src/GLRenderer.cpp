#include "GLRenderer.h"

#include <iostream>

#include "IO.h"
#include "mathlib.h"

namespace {
	void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
		const char* sourceStr = [&] {
			switch (source) {
				case GL_DEBUG_SOURCE_API_ARB: return "API";
				case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: return "shader compiler";
				case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: return "window system";
				case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: return "third party";
				case GL_DEBUG_SOURCE_APPLICATION_ARB: return "application";
				case GL_DEBUG_SOURCE_OTHER_ARB: return "other";
				default: return "unknown";
			}
		}();

		const char* typeStr = [&] {
			switch (type) {
				case GL_DEBUG_TYPE_ERROR_ARB: return "error";
				case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: return "deprecated behavior";
				case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: return "undefined behavior";
				case GL_DEBUG_TYPE_PERFORMANCE_ARB: return "performance";
				case GL_DEBUG_TYPE_PORTABILITY_ARB: return "portability";
				case GL_DEBUG_TYPE_OTHER_ARB: return "other";
				default: return "unknown";
			}
		}();

		const char* severityStr = [&] {
			switch (severity) {
				case GL_DEBUG_SEVERITY_HIGH_ARB: return "high";
				case GL_DEBUG_SEVERITY_MEDIUM_ARB: return "medium";
				case GL_DEBUG_SEVERITY_LOW_ARB: return "low";
				default: return "unknown";
			}
		}();

		std::cerr << "OpenGL debug callback: [" << severityStr << "|" << sourceStr << "|" << typeStr << "] " << std::string(message, length) << '\n';

		if (type == GL_DEBUG_TYPE_ERROR_ARB)
			__debugbreak();
	}
}

GLRenderer::GLRenderer() {
	// error callback
	if (GLEW_ARB_debug_output) {
		glDebugMessageCallbackARB(debugCallback, nullptr);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	}

	std::clog << "Setting rendering states ...\n";
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	glEnable(GL_MULTISAMPLE);

	////
	//// configure lighting for flashlight
	////

	//glEnable(GL_LIGHT0);

	//GLfloat lightPos[] = {0.0f, 0.0f, 0.0f, 1.0f};
	//glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	//GLfloat spotDir[] = {0.0f, 0.0f, -1.0f};
	//glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spotDir);
	//glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 25.0f);
	//glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 1.0f);
	//glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.01f);
	//glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.01f);
	//glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0001f);

	m_coordsProgram = gl::Program{
		gl::Shader(GL_VERTEX_SHADER, std::experimental::filesystem::path{"../src/shader/coords.vert"}),
		gl::Shader(GL_FRAGMENT_SHADER, std::experimental::filesystem::path{"../src/shader/coords.frag"})};
}

void GLRenderer::addRenderable(std::unique_ptr<IRenderable> renderable) {
	m_renderables.emplace_back(std::move(renderable));
}

void GLRenderer::resizeViewport(int width, int height) {
	glViewport(0, 0, width, height);
}

void GLRenderer::render(RenderSettings settings) {
	m_settings = settings;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (auto& renderable : m_renderables)
		renderable->render(m_settings);

	if (m_settings.renderCoords) {
		m_emptyVao.bind();
		m_coordsProgram.use();
		glUniformMatrix4fv(m_coordsProgram.uniformLocation("matrix"), 1, false, glm::value_ptr(m_settings.projection * m_settings.view));
		glDrawArrays(GL_LINES, 0, 12);
	}
}
