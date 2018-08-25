#pragma once

#include <GL/glew.h>
#include <GL/GL.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <vector>

#include "IRenderable.h"
#include "opengl/Program.h"
#include "opengl/VAO.h"

class GLRenderer {
public:
	GLRenderer();

	void addRenderable(std::unique_ptr<IRenderable> renderable);

	void resizeViewport(int width, int height);

	void beginFrame(RenderSettings settings);

	void render();
	void renderCoords();

private:
	struct Glew {
		Glew() {
			if (glewInit() != GLEW_OK)
				throw std::runtime_error("glew failed to initialize");
		}
	} m_glew;

	RenderSettings m_settings;

	std::vector<std::unique_ptr<IRenderable>> m_renderables;

	gl::VAO m_emptyVao;
	gl::Program m_coordsProgram;
};