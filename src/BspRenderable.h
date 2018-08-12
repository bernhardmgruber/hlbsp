#pragma once

#include <GL/glew.h>
#include <GL/GL.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <optional>

#include "bspdef.h"
#include "IRenderable.h"

class Bsp;
class Camera;

class BspRenderable : public IRenderable {
public:
	BspRenderable(const Bsp& bsp, const Camera& camera);
	~BspRenderable();

	virtual void render(const RenderSettings& settings) override;

private:
	void loadSkyTextures();

	void renderStaticGeometry(vec3 vPos);
	void renderBrushEntities(vec3 vPos);
	void renderDecals();
	void renderLeafOutlines();
	void renderLeafOutlines(const bsp30::Leaf& leaf);
	void renderFace(int iFace);                             // Renders a face (polygon) by the given index
	void renderLeaf(int iLeaf);                             // Renders a leaf of the BSP tree by rendering each face of the leaf by the given index
	void renderBSP(int iNode, int iCurrentLeaf, vec3 vPos); // Recursively walks through the BSP tree and draws it
	void renderBrushEntity(int iEntity, vec3 vPos);         // Renders a brush entity by rendering each face of the associated model by the given index

private:
	const Bsp* m_bsp;
	const Camera* m_camera;

	const RenderSettings* m_settings = nullptr;

	std::optional<GLuint> m_skyBoxDL;
	GLuint m_shaderProgram;
};