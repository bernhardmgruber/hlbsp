#pragma once

#include <boost/dynamic_bitset.hpp>

#include <optional>

#include "IRenderable.h"
#include "bspdef.h"
#include "mathlib.h"
#include "IRenderer.h"

class Bsp;
class Camera;
class Entity;

class BspRenderable : public IRenderable {
public:
	BspRenderable(render::IRenderer& renderer, const Bsp& bsp, const Camera& camera);
	~BspRenderable();

	virtual void render(const RenderSettings& settings) override;

private:
	void loadTextures();
	auto loadLightmaps() -> std::vector<std::vector<glm::vec2>>;
	void loadSkyTextures();

	void renderSkybox();
	auto renderStaticGeometry(glm::vec3 pos) -> std::vector<render::FaceRenderInfo>;
	//void renderLeafOutlines();
	void renderLeaf(int iLeaf, std::vector<render::FaceRenderInfo>& fri);                                                                  // Renders a leaf of the BSP tree by rendering each face of the leaf by the given index
	void renderBSP(int node, const boost::dynamic_bitset<std::uint8_t>& visList, glm::vec3 pos, std::vector<render::FaceRenderInfo>& fri); // Recursively walks through the BSP tree and draws it

	void buildBuffers(std::vector<std::vector<glm::vec2>>&& lmCoords);

private:
	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;
	};

	struct VertexWithLM : Vertex {
		glm::vec2 lightmapCoord;
	};

	render::IRenderer& m_renderer;
	const Bsp* m_bsp;
	const Camera* m_camera;

	const RenderSettings* m_settings = nullptr;

	std::optional<std::unique_ptr<render::ITexture>> m_skyboxTex;
	std::vector<std::unique_ptr<render::ITexture>> m_textures;
	std::unique_ptr<render::ITexture> m_lightmapAtlas;

	std::unique_ptr<render::IBuffer> m_staticGeometryVbo;
	std::unique_ptr<render::IBuffer> m_decalVbo;

	std::unique_ptr<render::IInputLayout> m_staticGeometryVao;
	std::unique_ptr<render::IInputLayout> m_decalVao;

	std::vector<unsigned int> vertexOffsets;

	mutable std::vector<bool> facesDrawn;
};