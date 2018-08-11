#pragma once

#include <GL/glew.h>
#include <GL/GL.h>
#include <glm/vec3.hpp>

#include <optional>

class Bsp;
class Hud;

struct RenderSettings {
	bool textures = true;
	bool lightmaps = true;
	bool polygons = false;

	bool renderStaticBSP = true;
	bool renderBrushEntities = true;
	bool renderSkybox = true;
	bool renderDecals = true;
	bool renderCoords = false;
	bool renderLeafOutlines = false;
	bool renderHUD = true;

	bool useShader = true;
	bool nightvision = false;
	bool flashlight = false;
};

class GLRenderer {
public:
	GLRenderer();

	void resizeViewport(int width, int height);

	void beginFrame(RenderSettings settings);

	void render(const Bsp& bsp, const glm::vec3 cameraPos);
	void renderHud(const Hud& hud, unsigned int width, unsigned int height, glm::vec3 cameraPos, glm::vec2 cameraAngles, glm::vec3 cameraView, double fps);
	void renderCoords();

private:
	void renderSkyBox(const Bsp& bsp, const glm::vec3 cameraPos);

private:
	RenderSettings m_settings;

	GLuint m_shaderProgram;
	GLuint m_font;
};