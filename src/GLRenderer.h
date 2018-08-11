#pragma once

#include <GL/glew.h>
#include <GL/GL.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <optional>

#include "Bsp.h"

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

	void beginFrame(RenderSettings settings, glm::mat4 viewMatrix);

	void render(const Bsp& bsp, const glm::vec3 cameraPos);
	void renderHud(const Hud& hud, unsigned int width, unsigned int height, glm::vec3 cameraPos, glm::vec2 cameraAngles, glm::vec3 cameraView, double fps);
	void renderCoords();

private:
	void renderSkyBox(const Bsp& bsp, const glm::vec3 cameraPos);
	void renderStaticGeometry(const Bsp& bsp, vec3 vPos);
	void renderBrushEntities(const Bsp& bsp, vec3 vPos);
	void renderDecals(const Bsp& bsp);
	void renderLeafOutlines(const Bsp& bsp);
	void renderLeafOutlines(const bsp30::Leaf& leaf);
	void renderFace(const Bsp& bsp, int iFace);                             // Renders a face (polygon) by the given index
	void renderLeaf(const Bsp& bsp, int iLeaf);                             // Renders a leaf of the BSP tree by rendering each face of the leaf by the given index
	void renderBSP(const Bsp& bsp, int iNode, int iCurrentLeaf, vec3 vPos); // Recursively walks through the BSP tree and draws it
	void renderBrushEntity(const Bsp& bsp, int iEntity, vec3 vPos);         // Renders a brush entity by rendering each face of the associated model by the given index

private:
	RenderSettings m_settings;

	GLuint m_shaderProgram;
	GLuint m_font;

	glm::mat4 m_projectionMatrix;
};