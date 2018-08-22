#pragma once

#include <GL/glew.h>
#include <GL/GL.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <memory>
#include <vector>

#include "IRenderable.h"
#include "opengl/Program.h"
#include "font.h"

class Hud;

class GLRenderer {
public:
	GLRenderer();

	void addRenderable(std::unique_ptr<IRenderable> renderable);

	void resizeViewport(int width, int height);

	void beginFrame(RenderSettings settings, glm::mat4 viewMatrix);

	void render();
	void renderHud(const Hud& hud, unsigned int width, unsigned int height, glm::vec3 cameraPos, float pitch, float yaw, glm::vec3 cameraView, double fps);
	void renderCoords();

private:
	RenderSettings m_settings;

	std::vector<std::unique_ptr<IRenderable>> m_renderables;

	gl::Program m_coordsProgram;
	gl::Program m_fontProgram;
	Font m_font;
	glm::mat4 m_projectionMatrix;
};