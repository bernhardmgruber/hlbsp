#pragma once

#include <GL/glew.h>
#include <GL/GL.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <boost/dynamic_bitset.hpp>

#include <optional>

#include "IRenderable.h"
#include "bspdef.h"
#include "opengl/Buffer.h"
#include "opengl/Program.h"
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
		GLint lmId;
		unsigned int offset;
		unsigned int count;
	};

	void loadSkyTextures();

	void renderSkybox();
	void renderStaticGeometry(vec3 pos);
	void renderBrushEntities(vec3 pos);
	void renderDecals();
	void renderLeafOutlines();
	void renderLeafOutlines(const bsp30::Leaf& leaf);
	void renderFace(int iFace, std::vector<FaceRenderInfo>& fri);                             // Renders a face (polygon) by the given index
	void renderLeaf(int iLeaf, std::vector<FaceRenderInfo>& fri);                             // Renders a leaf of the BSP tree by rendering each face of the leaf by the given index
	void renderBSP(int node, const boost::dynamic_bitset<std::uint8_t>& visList, vec3 pos, std::vector<FaceRenderInfo>& fri); // Recursively walks through the BSP tree and draws it
	void renderBrushEntity(const Entity& ent, vec3 pos);   // Renders a brush entity by rendering each face of the associated model by the given index
	void renderFri(std::vector<FaceRenderInfo> fri);

	void buildBuffers();

private:
	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;
		glm::vec2 lightmapCoord;
	};

	struct VertexWithLM : Vertex {
		glm::vec2 lightmapCoord;
	};

	const Bsp* m_bsp;
	const Camera* m_camera;

	const RenderSettings* m_settings = nullptr;

	gl::VAO m_skyBoxVao;
	std::optional<GLuint> m_skyboxTex;
	gl::Program m_skyboxProgram;
	gl::Program m_shaderProgram;
	std::vector<unsigned int> vertexOffsets;
	gl::VAO m_staticGeometryVao;
	gl::Buffer m_staticGeometryVbo;
	gl::VAO m_decalVao;
	gl::Buffer m_decalVbo;

	mutable std::vector<bool> facesDrawn;
};