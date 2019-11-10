#pragma once

enum class RenderAPI : int {
	OpenGL,
	Direct3D
};

namespace global {
	inline auto renderApi = RenderAPI::OpenGL;

	inline bool textures = true;
	inline bool lightmaps = true;
	inline bool polygons = false;

	inline bool renderStaticBSP = true;
	inline bool renderBrushEntities = true;
	inline bool renderSkybox = true;
	inline bool renderDecals = true;
	inline bool renderCoords = false;
	inline bool renderLeafOutlines = false;
	inline bool renderHUD = true;

	inline bool nightvision = false;
	inline bool flashlight = false;
}
