#pragma once

#include <glm/mat4x4.hpp>

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

	glm::mat4 matrix;
};

class IRenderable {
public:
	virtual void render(const RenderSettings& settings) = 0;
};