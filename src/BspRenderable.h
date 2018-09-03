#pragma once

#include <boost/dynamic_bitset.hpp>

#include <optional>

#include "IRenderable.h"
#include "bspdef.h"
#include "mathlib.h"
#include "opengl/Buffer.h"
#include "opengl/Program.h"
#include "opengl/Texture.h"
#include "opengl/VAO.h"

class Bsp;
class Camera;
class Entity;

class BspRenderable : public IRenderable {
public:
	BspRenderable(const Bsp& bsp, const Camera& camera);
	~BspRenderable();

	virtual void render(const RenderSettings& settings) override;

private:
	struct FaceRenderInfo {
		GLint texId;
		unsigned int offset;
		unsigned int count;
	};

	void loadTextures();
	auto loadLightmaps() -> std::vector<std::vector<glm::vec2>>;
	void loadSkyTextures();

	void renderSkybox();
	void renderStaticGeometry(glm::vec3 pos);
	void renderBrushEntities(glm::vec3 pos);
	void renderDecals();
	void renderLeafOutlines();
	void renderFace(int iFace, std::vector<FaceRenderInfo>& fri);                                                                  // Renders a face (polygon) by the given index
	void renderLeaf(int iLeaf, std::vector<FaceRenderInfo>& fri);                                                                  // Renders a leaf of the BSP tree by rendering each face of the leaf by the given index
	void renderBSP(int node, const boost::dynamic_bitset<std::uint8_t>& visList, glm::vec3 pos, std::vector<FaceRenderInfo>& fri); // Recursively walks through the BSP tree and draws it
	void renderBrushEntity(const Entity& ent, glm::vec3 pos);                                                                      // Renders a brush entity by rendering each face of the associated model by the given index
	void renderFri(std::vector<FaceRenderInfo> fri);

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

	const Bsp* m_bsp;
	const Camera* m_camera;

	const RenderSettings* m_settings = nullptr;

	gl::VAO m_skyBoxVao;
	std::optional<gl::Texture> m_skyboxTex;
	gl::Program m_skyboxProgram;

	std::vector<gl::Texture> m_textureIds;
	gl::Texture m_lightmapAtlasId;
	gl::Program m_shaderProgram;
	std::vector<unsigned int> vertexOffsets;
	gl::VAO m_staticGeometryVao;
	gl::Buffer m_staticGeometryVbo;

	gl::VAO m_decalVao;
	gl::Buffer m_decalVbo;

	mutable std::vector<bool> facesDrawn;
};