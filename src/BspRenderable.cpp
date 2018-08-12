#include "BspRenderable.h"

#include <iostream>

#include "glsl.h"
#include "Bsp.h"
#include "Camera.h"

BspRenderable::BspRenderable(const Bsp& bsp, const Camera& camera)
	: m_bsp(&bsp)
	, m_camera(&camera) {

	std::clog << "Loading bsp shaders ...\n";
	GLuint vsMain = glCreateShader(GL_VERTEX_SHADER);
	GLuint fsMain = glCreateShader(GL_FRAGMENT_SHADER);

	glslShaderSourceFile(vsMain, "../../src/shader/main.vert");
	glslShaderSourceFile(fsMain, "../../src/shader/main.frag");

	glCompileShader(vsMain);
	glCompileShader(fsMain);

	m_shaderProgram = glCreateProgram();
	glAttachShader(m_shaderProgram, vsMain);
	glAttachShader(m_shaderProgram, fsMain);
	glLinkProgram(m_shaderProgram);
	glslPrintProgramInfoLog(m_shaderProgram);
}

BspRenderable::~BspRenderable() {
	if (m_skyBoxDL)
		glDeleteLists(*m_skyBoxDL, 1);
}

void BspRenderable::loadSkyTextures() {
	std::clog << "Loading sky textures ...\n";

	const auto images = m_bsp->loadSkyBox();
	if (!images)
		return;

	GLuint nSkyTex[6];
	glGenTextures(6, nSkyTex);
	for (auto i = 0; i < 6; i++) {
		glBindTexture(GL_TEXTURE_2D, nSkyTex[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (*images)[i].width, (*images)[i].height, 0, (*images)[i].channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, (*images)[i].data.data());
	}

	//Create Displaylist
	m_skyBoxDL = glGenLists(1);
	glNewList(*m_skyBoxDL, GL_COMPILE);

	//http://enter.diehlsworld.de/ogl/skyboxartikel/skybox.htm
	glDepthMask(0); // prevent writing depth coords

	float fAHalf = 100; //half length of the edge of the cube

						//front
	glBindTexture(GL_TEXTURE_2D, nSkyTex[0]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(fAHalf, -fAHalf, -fAHalf);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(fAHalf, -fAHalf, fAHalf);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(-fAHalf, -fAHalf, fAHalf);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-fAHalf, -fAHalf, -fAHalf);
	glEnd();

	//back
	glBindTexture(GL_TEXTURE_2D, nSkyTex[1]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-fAHalf, fAHalf, -fAHalf);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-fAHalf, fAHalf, fAHalf);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(fAHalf, fAHalf, fAHalf);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(fAHalf, fAHalf, -fAHalf);
	glEnd();

	//right
	glBindTexture(GL_TEXTURE_2D, nSkyTex[2]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(fAHalf, fAHalf, -fAHalf);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(fAHalf, fAHalf, fAHalf);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(fAHalf, -fAHalf, fAHalf);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(fAHalf, -fAHalf, -fAHalf);
	glEnd();

	//left
	glBindTexture(GL_TEXTURE_2D, nSkyTex[3]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-fAHalf, -fAHalf, -fAHalf);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-fAHalf, -fAHalf, fAHalf);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(-fAHalf, fAHalf, fAHalf);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-fAHalf, fAHalf, -fAHalf);
	glEnd();

	//up
	glBindTexture(GL_TEXTURE_2D, nSkyTex[4]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(fAHalf, fAHalf, fAHalf);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-fAHalf, fAHalf, fAHalf);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(-fAHalf, -fAHalf, fAHalf);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(fAHalf, -fAHalf, fAHalf);
	glEnd();

	//down
	glBindTexture(GL_TEXTURE_2D, nSkyTex[5]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-fAHalf, fAHalf, -fAHalf);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(fAHalf, fAHalf, -fAHalf);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(fAHalf, -fAHalf, -fAHalf);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-fAHalf, -fAHalf, -fAHalf);
	glEnd();

	glDepthMask(1);

	glEndList();
}

void BspRenderable::render(const RenderSettings& settings) {
	m_settings = &settings;

	// Enable Shader
	glUseProgram(m_shaderProgram);

	glUniform1i(glGetUniformLocation(m_shaderProgram, "tex1"), 0);
	glUniform1i(glGetUniformLocation(m_shaderProgram, "tex2"), 1);
	glUniform1i(glGetUniformLocation(m_shaderProgram, "nightvision"), static_cast<GLint>(settings.nightvision));
	glUniform1i(glGetUniformLocation(m_shaderProgram, "flashlight"), static_cast<GLint>(settings.flashlight));
	
	const auto& cameraPos = m_camera->position();

	// render sky box
	if (m_skyBoxDL && settings.renderSkybox) {
		const auto matrix = glm::translate(m_settings->matrix, cameraPos);
		glUniform1i(glGetUniformLocation(m_shaderProgram, "unit1Enabled"), 1);
		glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "matrix"), 1, false, glm::value_ptr(matrix));
		glCallList(*m_skyBoxDL);
		glUniform1i(glGetUniformLocation(m_shaderProgram, "unit1Enabled"), 0);
	}

	glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "matrix"), 1, false, glm::value_ptr(settings.matrix));

	// turn on needed texture units
	glUniform1i(glGetUniformLocation(m_shaderProgram, "unit1Enabled"), static_cast<GLint>(settings.textures || settings.lightmaps));
	glUniform1i(glGetUniformLocation(m_shaderProgram, "unit2Enabled"), static_cast<GLint>(settings.textures && settings.lightmaps));

	glEnable(GL_DEPTH_TEST);

	if (settings.renderStaticBSP)
		renderStaticGeometry(cameraPos);

	if (settings.renderBrushEntities)
		renderBrushEntities(cameraPos);

	// Turn off second unit, if it was enabled
	glUniform1i(glGetUniformLocation(m_shaderProgram, "unit2Enabled"), 0);
	glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "matrix"), 1, false, glm::value_ptr(settings.matrix));

	if (settings.renderDecals) {
		glActiveTexture(GL_TEXTURE0_ARB);
		renderDecals();
	}

	// Turn off first unit, if it was enabled
	glUniform1i(glGetUniformLocation(m_shaderProgram, "unit1Enabled"), 0);

	glDisable(GL_DEPTH_TEST);

	glUseProgram(0);

	// Leaf outlines
	if (settings.renderLeafOutlines)
		renderLeafOutlines();
}

void BspRenderable::renderStaticGeometry(vec3 vPos) {
	for (auto&& b : m_bsp->facesDrawn)
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

	for (const auto& decal : m_bsp->decals) {
		glBindTexture(GL_TEXTURE_2D, decal.nTex);

		glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(0, 0);
		glNormal3f(decal.normal.x, decal.normal.y, decal.normal.z);
		glVertex3f(decal.vec[0].x, decal.vec[0].y, decal.vec[0].z);
		glTexCoord2f(1, 0);
		glNormal3f(decal.normal.x, decal.normal.y, decal.normal.z);
		glVertex3f(decal.vec[1].x, decal.vec[1].y, decal.vec[1].z);
		glTexCoord2f(1, 1);
		glNormal3f(decal.normal.x, decal.normal.y, decal.normal.z);
		glVertex3f(decal.vec[2].x, decal.vec[2].y, decal.vec[2].z);
		glTexCoord2f(0, 1);
		glNormal3f(decal.normal.x, decal.normal.y, decal.normal.z);
		glVertex3f(decal.vec[3].x, decal.vec[3].y, decal.vec[3].z);
		glEnd();
	}

	glDisable(GL_BLEND);
	glDisable(GL_POLYGON_OFFSET_FILL);
}


void BspRenderable::renderLeafOutlines()
{
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
	if (m_bsp->facesDrawn[face])
		return;
	m_bsp->facesDrawn[face] = true;

	if (m_bsp->faces[face].styles[0] == 0xFF)
		return;

	auto renderTriangle = [&](int i) {
		vec3 normal = m_bsp->planes[m_bsp->faces[face].planeIndex].normal;
		if (m_bsp->faces[face].planeSide)
			normal = -normal;
		glNormal3f(normal.x, normal.y, normal.z);

		int edge = m_bsp->surfEdges[m_bsp->faces[face].firstEdgeIndex + i];
		if (edge > 0)
			glVertex3f(m_bsp->vertices[m_bsp->edges[edge].vertexIndex[0]].x, m_bsp->vertices[m_bsp->edges[edge].vertexIndex[0]].y, m_bsp->vertices[m_bsp->edges[edge].vertexIndex[0]].z);
		else {
			edge *= -1;
			glVertex3f(m_bsp->vertices[m_bsp->edges[edge].vertexIndex[1]].x, m_bsp->vertices[m_bsp->edges[edge].vertexIndex[1]].y, m_bsp->vertices[m_bsp->edges[edge].vertexIndex[1]].z);
		}
	};

	// if the light map offset is not -1 and the lightmap lump is not empty, there are lightmaps
	bool bLightmapAvail = static_cast<signed>(m_bsp->faces[face].lightmapOffset) != -1 && m_bsp->header.lump[bsp30::LumpType::LUMP_LIGHTING].length > 0;

	if (bLightmapAvail && m_settings->lightmaps && m_settings->textures) {
		// We need both texture units for textures and lightmaps

		// base texture
		glActiveTexture(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_2D, m_bsp->textureIds[m_bsp->textureInfos[m_bsp->faces[face].textureInfo].miptexIndex]);

		// light map
		glActiveTexture(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, m_bsp->lightmapTexIds[face]);

		glBegin(GL_TRIANGLE_FAN);
		for (int i = 0; i < m_bsp->faces[face].edgeCount; i++) {
			glMultiTexCoord2f(GL_TEXTURE0_ARB, m_bsp->faceTexCoords[face].texCoords[i].s, m_bsp->faceTexCoords[face].texCoords[i].t);
			glMultiTexCoord2f(GL_TEXTURE1_ARB, m_bsp->faceTexCoords[face].lightmapCoords[i].s, m_bsp->faceTexCoords[face].lightmapCoords[i].t);
			renderTriangle(i);
		}
		glEnd();
	}
	else {
		// We need one texture unit for either textures or lightmaps
		glActiveTexture(GL_TEXTURE0_ARB);

		if (m_settings->lightmaps)
			glBindTexture(GL_TEXTURE_2D, m_bsp->lightmapTexIds[face]);
		else
			glBindTexture(GL_TEXTURE_2D, m_bsp->textureIds[m_bsp->textureInfos[m_bsp->faces[face].textureInfo].miptexIndex]);

		glBegin(GL_TRIANGLE_FAN);
		for (int i = 0; i < m_bsp->faces[face].edgeCount; i++) {
			if (m_settings->lightmaps)
				glTexCoord2f(m_bsp->faceTexCoords[face].lightmapCoords[i].s, m_bsp->faceTexCoords[face].lightmapCoords[i].t);
			else
				glTexCoord2f(m_bsp->faceTexCoords[face].texCoords[i].s, m_bsp->faceTexCoords[face].texCoords[i].t);
			renderTriangle(i);
		}
		glEnd();
	}
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
		default:             return glm::dot(m_bsp->planes[m_bsp->nodes[iNode].planeIndex].normal, vPos) - m_bsp->planes[m_bsp->nodes[iNode].planeIndex].dist;
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
	unsigned char nAlpha;
	if (const auto renderamt = ent.findProperty("renderamt"))
		nAlpha = std::stoi(*renderamt);
	else
		nAlpha = 255;

	// Rendermode
	unsigned char nRenderMode;
	if (const auto pszRenderMode = ent.findProperty("rendermode"))
		nRenderMode = std::stoi(*pszRenderMode);
	else
		nRenderMode = bsp30::RENDER_MODE_NORMAL;

	const auto matrix = glm::translate(m_settings->matrix, m_bsp->models[iModel].vOrigin);
	glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "matrix"), 1, false, glm::value_ptr(matrix));

	switch (nRenderMode) {
	case bsp30::RENDER_MODE_NORMAL:
		break;
	case bsp30::RENDER_MODE_TEXTURE:
		glColor4f(1.0f, 1.0f, 1.0f, static_cast<float>(nAlpha) / 255.0f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDepthMask(0u);

		glActiveTexture(GL_TEXTURE0_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		break;
	case bsp30::RENDER_MODE_SOLID:
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.25);
		break;
	case bsp30::RENDER_MODE_ADDITIVE:
		glColor4f(1.0f, 1.0f, 1.0f, static_cast<float>(nAlpha) / 255.0f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glDepthMask(0u);

		glActiveTexture(GL_TEXTURE0_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		break;
	}

	renderBSP(m_bsp->models[iModel].headNodesIndex[0], -1, vPos);

	switch (nRenderMode) {
	case bsp30::RENDER_MODE_NORMAL:
		break;
	case bsp30::RENDER_MODE_TEXTURE:
	case bsp30::RENDER_MODE_ADDITIVE:
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glDisable(GL_BLEND);
		glDepthMask(1u);

		glActiveTexture(GL_TEXTURE0_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		break;
	case bsp30::RENDER_MODE_SOLID:
		glDisable(GL_ALPHA_TEST);
		break;
	}
}
