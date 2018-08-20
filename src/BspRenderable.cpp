#include "BspRenderable.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#include <iostream>
#include <numeric>

#include "Bsp.h"
#include "Camera.h"
#include "glsl.h"

BspRenderable::BspRenderable(const Bsp& bsp, const Camera& camera)
	: m_bsp(&bsp), m_camera(&camera) {
	std::clog << "Loading bsp shaders ...\n";
	m_shaderProgram = gl::Program{
		gl::Shader(GL_VERTEX_SHADER, std::experimental::filesystem::path{"../../src/shader/main.vert"}),
		gl::Shader(GL_FRAGMENT_SHADER, std::experimental::filesystem::path{"../../src/shader/main.frag"}),
	};

	m_skyboxProgram = gl::Program{
		gl::Shader(GL_VERTEX_SHADER, std::experimental::filesystem::path{"../../src/shader/skybox.vert"}),
		gl::Shader(GL_FRAGMENT_SHADER, std::experimental::filesystem::path{"../../src/shader/skybox.frag"}),
	};

	buildBuffers();
	loadSkyTextures();

	facesDrawn.resize(bsp.faces.size());
}

BspRenderable::~BspRenderable() {
	if (m_skyboxTex)
		glDeleteTextures(0, &*m_skyboxTex);
}

void BspRenderable::loadSkyTextures() {
	const auto images = m_bsp->loadSkyBox();
	if (!images)
		return;

	glGenTextures(1, &m_skyboxTex.emplace());
	glBindTexture(GL_TEXTURE_CUBE_MAP, *m_skyboxTex);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	for (auto i = 0; i < 6; i++)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, (*images)[i].width, (*images)[i].height, 0, (*images)[i].channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, (*images)[i].data.data());
}

void BspRenderable::render(const RenderSettings& settings) {
	m_settings = &settings;

	// render sky box
	if (m_skyboxTex && settings.renderSkybox)
		renderSkybox();

	// Enable Shader
	m_shaderProgram.use();

	glUniform1i(m_shaderProgram.uniformLocation("tex1"), 0);
	glUniform1i(m_shaderProgram.uniformLocation("tex2"), 1);
	glUniform1i(m_shaderProgram.uniformLocation("nightvision"), static_cast<GLint>(settings.nightvision));
	//glUniform1i(m_shaderProgram.uniformLocation("flashlight"), static_cast<GLint>(settings.flashlight));

	const auto& cameraPos = m_camera->position();

	const auto matrix = settings.projection * settings.view;
	glUniformMatrix4fv(m_shaderProgram.uniformLocation("matrix"), 1, false, glm::value_ptr(matrix));

	// turn on needed texture units
	glUniform1i(m_shaderProgram.uniformLocation("unit1Enabled"), static_cast<GLint>(settings.textures));
	glUniform1i(m_shaderProgram.uniformLocation("unit2Enabled"), static_cast<GLint>(settings.lightmaps));

	glEnable(GL_DEPTH_TEST);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexWithLM), reinterpret_cast<void*>(offsetof(VertexWithLM, position)));
	glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(VertexWithLM), reinterpret_cast<void*>(offsetof(VertexWithLM, normal)));
	glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(VertexWithLM), reinterpret_cast<void*>(offsetof(VertexWithLM, texCoord)));
	glVertexAttribPointer(3, 2, GL_FLOAT, false, sizeof(VertexWithLM), reinterpret_cast<void*>(offsetof(VertexWithLM, lightmapCoord)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	if (settings.renderStaticBSP)
		renderStaticGeometry(cameraPos);

	if (settings.renderBrushEntities)
		renderBrushEntities(cameraPos);

	glDisableVertexAttribArray(3);

	glUniform1i(m_shaderProgram.uniformLocation("unit2Enabled"), 0);
	glUniformMatrix4fv(m_shaderProgram.uniformLocation("matrix"), 1, false, glm::value_ptr(matrix));

	glBindBuffer(GL_ARRAY_BUFFER, m_decalVbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
	glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
	glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, texCoord)));

	if (settings.renderDecals)
		renderDecals();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);


	// Turn off first unit, if it was enabled
	glUniform1i(m_shaderProgram.uniformLocation("unit1Enabled"), 0);

	glDisable(GL_DEPTH_TEST);

	glUseProgram(0);

	// Leaf outlines
	if (settings.renderLeafOutlines)
		renderLeafOutlines();
}

void BspRenderable::renderSkybox() {
	auto matrix = m_settings->projection * glm::eulerAngleXZX(degToRad(-m_settings->pitch - 90.0f), degToRad(-m_settings->yaw), degToRad(+90.0f));

	m_skyboxProgram.use();
	glUniform1i(m_skyboxProgram.uniformLocation("cubeSampler"), 0);
	glUniformMatrix4fv(m_skyboxProgram.uniformLocation("matrix"), 1, false, glm::value_ptr(matrix));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, *m_skyboxTex);

	glDepthMask(GL_FALSE);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);
}

void BspRenderable::renderStaticGeometry(vec3 vPos) {
	for (auto&& b : facesDrawn)
		b = false;

	const int iLeaf = m_bsp->findLeaf(vPos); //Get the leaf where the camera is in
	renderBSP(0, iLeaf, vPos);
}

void BspRenderable::renderBrushEntities(vec3 vPos) {
	for (int i = 0; i < m_bsp->brushEntities.size(); i++) // TODO(bernh): implement PVS for pEntities
		renderBrushEntity(i, vPos);
}

void BspRenderable::renderDecals() {
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.0f, -2.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0);

	for (auto i = 0; i < m_bsp->decals.size(); i++) {
		glBindTexture(GL_TEXTURE_2D, m_bsp->decals[i].nTex);
		glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
	}

	glDisable(GL_BLEND);
	glDisable(GL_POLYGON_OFFSET_FILL);
}


void BspRenderable::renderLeafOutlines() {
	glLineWidth(1.0f);
	glLineStipple(1, 0xF0F0);
	glEnable(GL_LINE_STIPPLE);
	for (const auto& leaf : m_bsp->leaves)
		renderLeafOutlines(leaf);
	glDisable(GL_LINE_STIPPLE);
	glColor3f(1, 1, 1);
}

void BspRenderable::renderLeafOutlines(const bsp30::Leaf& leaf) {
	srand(reinterpret_cast<unsigned>(&leaf));
	glColor3ub(rand() % 255, rand() % 255, rand() % 255);

	glBegin(GL_LINES);
	// Draw right face of bounding box
	glVertex3f(leaf.upper[0], leaf.upper[1], leaf.upper[2]);
	glVertex3f(leaf.upper[0], leaf.lower[1], leaf.upper[2]);
	glVertex3f(leaf.upper[0], leaf.lower[1], leaf.upper[2]);
	glVertex3f(leaf.upper[0], leaf.lower[1], leaf.lower[2]);
	glVertex3f(leaf.upper[0], leaf.lower[1], leaf.lower[2]);
	glVertex3f(leaf.upper[0], leaf.upper[1], leaf.lower[2]);
	glVertex3f(leaf.upper[0], leaf.upper[1], leaf.lower[2]);
	glVertex3f(leaf.upper[0], leaf.upper[1], leaf.upper[2]);

	// Draw left face of bounding box
	glVertex3f(leaf.lower[0], leaf.lower[1], leaf.lower[2]);
	glVertex3f(leaf.lower[0], leaf.upper[1], leaf.lower[2]);
	glVertex3f(leaf.lower[0], leaf.upper[1], leaf.lower[2]);
	glVertex3f(leaf.lower[0], leaf.upper[1], leaf.upper[2]);
	glVertex3f(leaf.lower[0], leaf.upper[1], leaf.upper[2]);
	glVertex3f(leaf.lower[0], leaf.lower[1], leaf.upper[2]);
	glVertex3f(leaf.lower[0], leaf.lower[1], leaf.upper[2]);
	glVertex3f(leaf.lower[0], leaf.lower[1], leaf.lower[2]);

	// Connect the faces
	glVertex3f(leaf.lower[0], leaf.upper[1], leaf.upper[2]);
	glVertex3f(leaf.upper[0], leaf.upper[1], leaf.upper[2]);
	glVertex3f(leaf.lower[0], leaf.upper[1], leaf.lower[2]);
	glVertex3f(leaf.upper[0], leaf.upper[1], leaf.lower[2]);
	glVertex3f(leaf.lower[0], leaf.lower[1], leaf.lower[2]);
	glVertex3f(leaf.upper[0], leaf.lower[1], leaf.lower[2]);
	glVertex3f(leaf.lower[0], leaf.lower[1], leaf.upper[2]);
	glVertex3f(leaf.upper[0], leaf.lower[1], leaf.upper[2]);
	glEnd();
}


void BspRenderable::renderFace(int face) {
	if (facesDrawn[face])
		return;
	facesDrawn[face] = true;

	if (m_bsp->faces[face].styles[0] == 0xFF)
		return;

	// if the light map offset is not -1 and the lightmap lump is not empty, there are lightmaps
	const bool lightmapAvailable = static_cast<signed>(m_bsp->faces[face].lightmapOffset) != -1 && m_bsp->header.lump[bsp30::LumpType::LUMP_LIGHTING].length > 0;

	if (m_settings->textures) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_bsp->textureIds[m_bsp->textureInfos[m_bsp->faces[face].textureInfo].miptexIndex]);
	}

	if (m_settings->lightmaps && lightmapAvailable) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_bsp->lightmapTexIds[face]);
	}

	glDrawArrays(GL_TRIANGLE_FAN, vertexOffsets[face], m_bsp->faces[face].edgeCount);
}

void BspRenderable::renderLeaf(int leaf) {
	for (int i = 0; i < m_bsp->leaves[leaf].markSurfaceCount; i++)
		renderFace(m_bsp->markSurfaces[m_bsp->leaves[leaf].firstMarkSurface + i]);
}

void BspRenderable::renderBSP(int iNode, int iCurrentLeaf, vec3 vPos) {
	if (iNode < 0) {
		if (iNode == -1)
			return;

		if (iCurrentLeaf > 0)
			if (!m_bsp->visLists.empty() && !m_bsp->visLists[iCurrentLeaf - 1].empty() && !m_bsp->visLists[iCurrentLeaf - 1][~iNode - 1])
				return;

		renderLeaf(~iNode);

		return;
	}

	const auto dist = [&] {
		switch (m_bsp->planes[m_bsp->nodes[iNode].planeIndex].type) {
			case bsp30::PLANE_X: return vPos.x - m_bsp->planes[m_bsp->nodes[iNode].planeIndex].dist;
			case bsp30::PLANE_Y: return vPos.y - m_bsp->planes[m_bsp->nodes[iNode].planeIndex].dist;
			case bsp30::PLANE_Z: return vPos.z - m_bsp->planes[m_bsp->nodes[iNode].planeIndex].dist;
			default: return glm::dot(m_bsp->planes[m_bsp->nodes[iNode].planeIndex].normal, vPos) - m_bsp->planes[m_bsp->nodes[iNode].planeIndex].dist;
		}
	}();

	const auto child1 = dist > 0 ? 1 : 0;
	const auto child2 = dist > 0 ? 0 : 1;
	renderBSP(m_bsp->nodes[iNode].childIndex[child1], iCurrentLeaf, vPos);
	renderBSP(m_bsp->nodes[iNode].childIndex[child2], iCurrentLeaf, vPos);
}

void BspRenderable::renderBrushEntity(int iEntity, vec3 vPos) {
	const auto& ent = m_bsp->entities[m_bsp->brushEntities[iEntity]];

	// Model
	int iModel = std::stoi(ent.findProperty("model")->substr(1));

	// Alpha value
	const auto alpha = [&] {
		if (const auto renderamt = ent.findProperty("renderamt"))
			return std::stoi(*renderamt) / 255.0f;
		return 1.0f;
	}();

	// Rendermode
	unsigned char nRenderMode;
	if (const auto pszRenderMode = ent.findProperty("rendermode"))
		nRenderMode = std::stoi(*pszRenderMode);
	else
		nRenderMode = bsp30::RENDER_MODE_NORMAL;

	const auto matrix = glm::translate(m_settings->projection * m_settings->view, m_bsp->models[iModel].vOrigin);
	glUniformMatrix4fv(m_shaderProgram.uniformLocation("matrix"), 1, false, glm::value_ptr(matrix));

	switch (nRenderMode) {
		case bsp30::RENDER_MODE_NORMAL:
			break;
		case bsp30::RENDER_MODE_TEXTURE:
			glColor4f(1.0f, 1.0f, 1.0f, alpha);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glDepthMask(GL_FALSE);
			break;
		case bsp30::RENDER_MODE_SOLID:
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.25);
			break;
		case bsp30::RENDER_MODE_ADDITIVE:
			glColor4f(1.0f, 1.0f, 1.0f, alpha);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			glDepthMask(GL_FALSE);
			break;
	}

	renderBSP(m_bsp->models[iModel].headNodesIndex[0], -1, vPos);

	switch (nRenderMode) {
		case bsp30::RENDER_MODE_NORMAL:
			break;
		case bsp30::RENDER_MODE_TEXTURE:
		case bsp30::RENDER_MODE_ADDITIVE:
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
			break;
		case bsp30::RENDER_MODE_SOLID:
			glDisable(GL_ALPHA_TEST);
			break;
	}
}

void BspRenderable::buildBuffers() {
	{
		// static and brush geometry
		std::vector<VertexWithLM> vertices;

		for (const auto& face : m_bsp->faces) {
			const auto faceIndex = &face - &m_bsp->faces.front();
			for (int i = 0; i < face.edgeCount; i++) {
				auto& v = vertices.emplace_back();

				const auto& coords = m_bsp->faceTexCoords[faceIndex];
				v.texCoord = coords.texCoords[i];
				v.lightmapCoord = coords.lightmapCoords.empty() ? glm::vec2{0.0} : coords.lightmapCoords[i];

				v.normal = m_bsp->planes[face.planeIndex].normal;
				if (face.planeSide)
					v.normal = -v.normal;

				int edge = m_bsp->surfEdges[face.firstEdgeIndex + i];
				if (edge > 0)
					v.position = m_bsp->vertices[m_bsp->edges[edge].vertexIndex[0]];
				else
					v.position = m_bsp->vertices[m_bsp->edges[-edge].vertexIndex[1]];
			}
			vertexOffsets.push_back(face.edgeCount);
		}

		std::exclusive_scan(begin(vertexOffsets), end(vertexOffsets), begin(vertexOffsets), 0);

		glGenBuffers(1, &m_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexWithLM), vertices.data(), GL_STATIC_DRAW);
	}

	{
		// decals
		std::vector<Vertex> vertices;

		for (const auto& decal : m_bsp->decals) {
			for (auto i = 0; i < 4; i++) {
				auto& v = vertices.emplace_back();
				v.position = decal.vec[i];
				v.normal = decal.normal;
				if (i == 0) v.texCoord = glm::vec2(0, 0);
				if (i == 1) v.texCoord = glm::vec2(1, 0);
				if (i == 2) v.texCoord = glm::vec2(1, 1);
				if (i == 3) v.texCoord = glm::vec2(0, 1);
			}
		}

		glGenBuffers(1, &m_decalVbo);
		glBindBuffer(GL_ARRAY_BUFFER, m_decalVbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	}
}
