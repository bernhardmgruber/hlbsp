#pragma once

#include <GL/glew.h>
#include <GL/GL.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <optional>

#include "IRenderable.h"
#include "bspdef.h"
#include "opengl/Program.h"

class Bsp;
class Camera;

class BspRenderable : public IRenderable {
public:
	BspRenderable(const Bsp& bsp, const Camera& camera);
	~BspRenderable();

	virtual void render(const RenderSettings& settings) override;

private:
	void loadSkyTextures();

	void renderSkybox();
	void renderStaticGeometry(vec3 vPos);
	void renderBrushEntities(vec3 vPos);
	void renderDecals();
	void renderLeafOutlines();
	void renderLeafOutlines(const bsp30::Leaf& leaf);
	void renderFace(int iFace);                             // Renders a face (polygon) by the given index
	void renderLeaf(int iLeaf);                             // Renders a leaf of the BSP tree by rendering each face of the leaf by the given index
	void renderBSP(int iNode, int iCurrentLeaf, vec3 vPos); // Recursively walks through the BSP tree and draws it
	void renderBrushEntity(int iEntity, vec3 vPos);         // Renders a brush entity by rendering each face of the associated model by the given index

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

	std::optional<GLuint> m_skyboxTex;
	gl::Program m_skyboxProgram;
	gl::Program m_shaderProgram;
	std::vector<unsigned int> vertexOffsets;
	GLuint m_vbo = 0;
	GLuint m_decalVbo = 0;

	mutable std::vector<bool> facesDrawn;
};