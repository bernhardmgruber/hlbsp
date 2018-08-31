#include "BspRenderable.h"

#include <iostream>
#include <numeric>
#include <random>

#include "Bsp.h"
#include "Camera.h"
#include "mathlib.h"

namespace {
	auto channelsToTextureType(const Image& img) {
		switch (img.channels) {
			case 1: return GL_RED;
			case 2: return GL_RG;
			case 3: return GL_RGB;
			case 4: return GL_RGBA;
			default: assert(false);
		}
	}
}

BspRenderable::BspRenderable(const Bsp& bsp, const Camera& camera)
	: m_bsp(&bsp), m_camera(&camera) {
	std::clog << "Loading bsp shaders ...\n";
	m_shaderProgram = gl::Program{
		gl::Shader(GL_VERTEX_SHADER, std::experimental::filesystem::path{"../src/shader/main.vert"}),
		gl::Shader(GL_FRAGMENT_SHADER, std::experimental::filesystem::path{"../src/shader/main.frag"}),
	};

	m_skyboxProgram = gl::Program{
		gl::Shader(GL_VERTEX_SHADER, std::experimental::filesystem::path{"../src/shader/skybox.vert"}),
		gl::Shader(GL_FRAGMENT_SHADER, std::experimental::filesystem::path{"../src/shader/skybox.frag"}),
	};

	loadSkyTextures();
	loadTextures();
	loadLightmaps();
	buildBuffers();

	facesDrawn.resize(bsp.faces.size());
}

BspRenderable::~BspRenderable() = default;

void BspRenderable::loadTextures() {
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	const auto& mipTexs = m_bsp->textures();

	m_textureIds.reserve(mipTexs.size());
	for (const auto& mipTex : mipTexs) {
		m_textureIds.emplace_back().bind(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, bsp30::MIPLEVELS - 1);
		for (int j = 0; j < bsp30::MIPLEVELS; j++)
			glTexImage2D(GL_TEXTURE_2D, j, GL_RGBA, mipTex.Img[j].width, mipTex.Img[j].height, 0, channelsToTextureType(mipTex.Img[j]), GL_UNSIGNED_BYTE, mipTex.Img[j].data.data());
	}
}

void BspRenderable::loadLightmaps() {
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	const auto& lightmaps = m_bsp->lightmaps();

	m_lightmapIds.reserve(lightmaps.size());
	for (const auto& lm : lightmaps) {
		m_lightmapIds.emplace_back().bind(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, lm.width, lm.height, 0, GL_RGB, GL_UNSIGNED_BYTE, lm.data.data());
	}
}

void BspRenderable::loadSkyTextures() {
	const auto images = m_bsp->loadSkyBox();
	if (!images)
		return;

	m_skyboxTex.emplace().bind(GL_TEXTURE_CUBE_MAP);
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

	m_shaderProgram.use();
	glUniform1i(m_shaderProgram.uniformLocation("tex1"), 0);
	glUniform1i(m_shaderProgram.uniformLocation("tex2"), 1);
	glUniform1i(m_shaderProgram.uniformLocation("nightvision"), static_cast<GLint>(settings.nightvision));
	//glUniform1i(m_shaderProgram.uniformLocation("flashlight"), static_cast<GLint>(settings.flashlight));
	glUniform1i(m_shaderProgram.uniformLocation("unit1Enabled"), static_cast<GLint>(settings.textures));
	glUniform1i(m_shaderProgram.uniformLocation("unit2Enabled"), static_cast<GLint>(settings.lightmaps));

	glUniform1i(m_shaderProgram.uniformLocation("alphaTest"), 0);

	const auto& cameraPos = m_camera->position;

	const auto matrix = settings.projection * settings.view;
	glUniformMatrix4fv(m_shaderProgram.uniformLocation("matrix"), 1, false, glm::value_ptr(matrix));

	glEnable(GL_DEPTH_TEST);

	if (settings.renderStaticBSP || settings.renderBrushEntities)
		m_staticGeometryVao.bind();

	if (settings.renderStaticBSP)
		renderStaticGeometry(cameraPos);

	if (settings.renderBrushEntities)
		renderBrushEntities(cameraPos);

	glUniform1i(m_shaderProgram.uniformLocation("unit2Enabled"), 0);
	glUniformMatrix4fv(m_shaderProgram.uniformLocation("matrix"), 1, false, glm::value_ptr(matrix));

	if (settings.renderDecals)
		renderDecals();

	glDisable(GL_DEPTH_TEST);

	glBindVertexArray(0);
	glUseProgram(0);

	// Leaf outlines
	if (settings.renderLeafOutlines) {
		glLoadMatrixf(glm::value_ptr(matrix));
		renderLeafOutlines();
	}
}

void BspRenderable::renderSkybox() {
	// TODO: glm in WSL Ubuntu does not yet have this function
	//auto matrix = m_settings->projection * glm::eulerAngleXZX(degToRad(-m_settings->pitch - 90.0f), degToRad(-m_settings->yaw), degToRad(+90.0f));
	auto matrix = m_settings->projection * glm::eulerAngleX(degToRad(-m_settings->pitch - 90.0f)) * glm::eulerAngleZ(degToRad(-m_settings->yaw)) * glm::eulerAngleX(degToRad(+90.0f));

	m_skyBoxVao.bind();
	m_skyboxProgram.use();
	glUniform1i(m_skyboxProgram.uniformLocation("cubeSampler"), 0);
	glUniformMatrix4fv(m_skyboxProgram.uniformLocation("matrix"), 1, false, glm::value_ptr(matrix));

	glActiveTexture(GL_TEXTURE0);
	m_skyboxTex->bind(GL_TEXTURE_CUBE_MAP);

	glDepthMask(GL_FALSE);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);
}

void BspRenderable::renderStaticGeometry(glm::vec3 pos) {
	for (auto&& b : facesDrawn)
		b = false;

	std::vector<FaceRenderInfo> fri;
	const auto leaf = m_bsp->findLeaf(pos);
	renderBSP(0, !leaf || m_bsp->visLists.empty() ? boost::dynamic_bitset<std::uint8_t>{} : m_bsp->visLists[*leaf - 1], pos, fri);
	renderFri(std::move(fri));
}

void BspRenderable::renderBrushEntities(glm::vec3 pos) {
	for (const auto i : m_bsp->brushEntities)
		renderBrushEntity(m_bsp->entities[i], pos);
}

void BspRenderable::renderDecals() {
	m_decalVao.bind();

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.0f, -2.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0);

	const auto& decals = m_bsp->decals();
	for (auto i = 0; i < decals.size(); i++) {
		glBindTexture(GL_TEXTURE_2D, m_textureIds[decals[i].texIndex].id());
		glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
	}

	glDisable(GL_BLEND);
	glDisable(GL_POLYGON_OFFSET_FILL);
}


void BspRenderable::renderLeafOutlines() {
	std::mt19937 engine;
	std::uniform_real_distribution dist(0.0f, 1.0f);

	glLineWidth(1.0f);
	glLineStipple(1, 0xF0F0);
	glEnable(GL_LINE_STIPPLE);
	for (const auto& leaf : m_bsp->leaves) {
		glColor3f(dist(engine), dist(engine), dist(engine));

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
	glDisable(GL_LINE_STIPPLE);
	glColor3f(1, 1, 1);
}

void BspRenderable::renderFace(int face, std::vector<FaceRenderInfo>& fri) {
	if (facesDrawn[face])
		return;
	facesDrawn[face] = true;

	if (m_bsp->faces[face].styles[0] == 0xFF)
		return;

	// if the light map offset is not -1 and the lightmap lump is not empty, there are lightmaps
	const bool lightmapAvailable = static_cast<signed>(m_bsp->faces[face].lightmapOffset) != -1 && m_bsp->header.lump[bsp30::LumpType::LUMP_LIGHTING].length > 0;

	auto& i = fri.emplace_back();
	if (m_settings->textures)
		i.texId = m_textureIds[m_bsp->textureInfos[m_bsp->faces[face].textureInfo].miptexIndex].id();
	else
		i.texId = 0;

	if (m_settings->lightmaps && lightmapAvailable)
		i.lmId = m_lightmapIds[face].id();
	else
		i.lmId = 0;

	i.offset = vertexOffsets[face];
	i.count = m_bsp->faces[face].edgeCount;
}

void BspRenderable::renderLeaf(int leaf, std::vector<FaceRenderInfo>& fri) {
	for (int i = 0; i < m_bsp->leaves[leaf].markSurfaceCount; i++)
		renderFace(m_bsp->markSurfaces[m_bsp->leaves[leaf].firstMarkSurface + i], fri);
}

void BspRenderable::renderBSP(int node, const boost::dynamic_bitset<std::uint8_t>& visList, glm::vec3 pos, std::vector<FaceRenderInfo>& fri) {
	if (node < 0) {
		if (node == -1)
			return;

		const int leaf = ~node;
		if (!visList.empty() && !visList[leaf - 1])
			return;

		renderLeaf(leaf, fri);

		return;
	}

	const auto dist = [&] {
		switch (m_bsp->planes[m_bsp->nodes[node].planeIndex].type) {
			case bsp30::PLANE_X: return pos.x - m_bsp->planes[m_bsp->nodes[node].planeIndex].dist;
			case bsp30::PLANE_Y: return pos.y - m_bsp->planes[m_bsp->nodes[node].planeIndex].dist;
			case bsp30::PLANE_Z: return pos.z - m_bsp->planes[m_bsp->nodes[node].planeIndex].dist;
			default: return glm::dot(m_bsp->planes[m_bsp->nodes[node].planeIndex].normal, pos) - m_bsp->planes[m_bsp->nodes[node].planeIndex].dist;
		}
	}();

	const auto child1 = dist > 0 ? 1 : 0;
	const auto child2 = dist > 0 ? 0 : 1;
	renderBSP(m_bsp->nodes[node].childIndex[child1], visList, pos, fri);
	renderBSP(m_bsp->nodes[node].childIndex[child2], visList, pos, fri);
}

void BspRenderable::renderBrushEntity(const Entity& ent, glm::vec3 pos) {
	const int model = std::stoi(ent.findProperty("model")->substr(1));

	const auto alpha = [&] {
		if (const auto renderamt = ent.findProperty("renderamt"))
			return std::stoi(*renderamt) / 255.0f;
		return 1.0f;
	}();

	const auto renderMode = [&] {
		if (const auto pszRenderMode = ent.findProperty("rendermode"))
			return static_cast<bsp30::RenderMode>(std::stoi(*pszRenderMode));
		else
			return bsp30::RENDER_MODE_NORMAL;
	}();

	const auto matrix = glm::translate(m_settings->projection * m_settings->view, m_bsp->models[model].origin);
	glUniformMatrix4fv(m_shaderProgram.uniformLocation("matrix"), 1, false, glm::value_ptr(matrix));

	switch (renderMode) {
		case bsp30::RENDER_MODE_NORMAL:
			break;
		case bsp30::RENDER_MODE_TEXTURE:
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glDepthMask(GL_FALSE);
			break;
		case bsp30::RENDER_MODE_SOLID:
			glUniform1i(m_shaderProgram.uniformLocation("alphaTest"), 1);
			break;
		case bsp30::RENDER_MODE_ADDITIVE:
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			glDepthMask(GL_FALSE);
			break;
	}

	std::vector<FaceRenderInfo> fri;
	renderBSP(m_bsp->models[model].headNodesIndex[0], boost::dynamic_bitset<uint8_t>{}, pos, fri); // for some odd reason, VIS does not work for entities ...
	renderFri(std::move(fri));

	switch (renderMode) {
		case bsp30::RENDER_MODE_NORMAL:
			break;
		case bsp30::RENDER_MODE_TEXTURE:
		case bsp30::RENDER_MODE_ADDITIVE:
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
			break;
		case bsp30::RENDER_MODE_SOLID:
			glUniform1i(m_shaderProgram.uniformLocation("alphaTest"), 0);
			break;
	}
}

void BspRenderable::renderFri(std::vector<FaceRenderInfo> fri) {
	// TODO: sort takes too much CPU time, FPS dropped by 20%
	//// sort by texture id to avoid some rebinds
	//std::sort(begin(fri), end(fri), [](const FaceRenderInfo& a, const FaceRenderInfo& b) {
	//	return a.texId < b.texId;
	//});

	for (const auto& i : fri) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, i.texId);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, i.lmId);
		glDrawArrays(GL_TRIANGLE_FAN, i.offset, i.count);
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

		// TODO clang does not yet have exclusive_scan
		//std::exclusive_scan(begin(vertexOffsets), end(vertexOffsets), begin(vertexOffsets), 0);
		auto sum = 0u;
		for (auto i = 0u; i < vertexOffsets.size(); i++) {
			const auto val = vertexOffsets[i];
			vertexOffsets[i] = sum;
			sum += val;
		}

		m_staticGeometryVao.bind();
		m_staticGeometryVbo.bind(GL_ARRAY_BUFFER);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexWithLM), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexWithLM), reinterpret_cast<void*>(offsetof(VertexWithLM, position)));
		glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(VertexWithLM), reinterpret_cast<void*>(offsetof(VertexWithLM, normal)));
		glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(VertexWithLM), reinterpret_cast<void*>(offsetof(VertexWithLM, texCoord)));
		glVertexAttribPointer(3, 2, GL_FLOAT, false, sizeof(VertexWithLM), reinterpret_cast<void*>(offsetof(VertexWithLM, lightmapCoord)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
	}

	{
		// decals
		std::vector<Vertex> vertices;

		for (const auto& decal : m_bsp->decals()) {
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

		m_decalVao.bind();
		m_decalVbo.bind(GL_ARRAY_BUFFER);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
		glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
		glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, texCoord)));
	}

	glBindVertexArray(0);
}
