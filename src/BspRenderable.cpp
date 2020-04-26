#include "BspRenderable.h"

#include <iostream>
#include <numeric>
#include <random>

#include "Bsp.h"
#include "Camera.h"
#include "mathlib.h"
#include "global.h"

namespace {
	class TextureAtlas {
	public:
		TextureAtlas(unsigned int width, unsigned int height, unsigned int channels = 3)
			: m_img(width, height, channels), allocated(width) {}

		auto store(const Image& image) -> glm::uvec2 {
			if (image.channels != m_img.channels)
				throw std::logic_error("image and atlas channel count mismatch");

			const auto loc = allocLightmap(image.width, image.height);
			if (!loc)
				throw std::runtime_error("atlas is full");

			for (auto y = 0u; y < image.height; y++) {
				const auto src = &image.data[(y * image.width) * image.channels];
				const auto dst = &m_img.data[((loc->y + y) * m_img.width + loc->x) * image.channels];
				std::copy(src, src + image.width * image.channels, dst);
			}

			return *loc;
		}

		auto convertCoord(const Image& image, glm::uvec2 storedPos, glm::vec2 coord) {
			return (glm::vec2(storedPos) + coord * glm::vec2(image.width, image.height)) / glm::vec2(m_img.width, m_img.height);
		}

		auto img() const -> const auto& { return m_img; }

	private:
		// from: http://fabiensanglard.net/quake2/quake2_opengl_renderer.php
		// based on Quake 2
		auto allocLightmap(unsigned int lmWidth, unsigned int lmHeight) -> std::optional<glm::uvec2> {
			glm::uvec2 pos(0, 0);

			auto best = m_img.height;
			for (auto i = 0u; i < m_img.width - lmWidth; i++) {
				auto best2 = 0u;
				auto j = 0u;
				for (j = 0u; j < lmWidth; j++) {
					if (allocated[i + j] >= best)
						break;
					if (allocated[i + j] > best2)
						best2 = allocated[i + j];
				}
				if (j == lmWidth) {
					pos.x = i;
					pos.y = best = best2;
				}
			}

			if (best + lmHeight > m_img.height)
				return {};

			for (unsigned int i = 0; i < lmWidth; i++)
				allocated[pos.x + i] = best + lmHeight;

			return pos;
		}

		std::vector<unsigned int> allocated;
		Image m_img;
	};
}

BspRenderable::BspRenderable(render::IRenderer& renderer, const Bsp& bsp, const Camera& camera)
	: m_renderer(renderer), m_bsp(&bsp), m_camera(&camera) {
	loadSkyTextures();
	loadTextures();
	auto lmCoords = loadLightmaps();
	buildBuffers(std::move(lmCoords));

	facesDrawn.resize(bsp.faces.size());
}

BspRenderable::~BspRenderable() = default;

void BspRenderable::loadTextures() {
	const auto& mipTexs = m_bsp->m_textures;

	//// create texture atlas
	//TextureAtlas atlas(2048, 2048, 4);
	//std::vector<glm::uvec2> lmPositions(mipTexs.size());
	//for (auto i = 0u; i < mipTexs.size(); i++) {
	//	const auto& lm = mipTexs[i].Img[0];
	//	if (lm.width == 0 || lm.height == 0)
	//		continue;
	//	lmPositions[i] = atlas.store(lm);
	//}
	//atlas.img().Save("tex_atlas.png");

	m_textures.reserve(mipTexs.size());
	for (const auto& mipTex : mipTexs)
		m_textures.emplace_back(m_renderer.createTexture(std::vector<Image>{mipTex.Img, mipTex.Img + 4}));
}

auto BspRenderable::loadLightmaps() -> std::vector<std::vector<glm::vec2>> {
	const auto& lightmaps = m_bsp->m_lightmaps;

	// create lightmap atlas
	TextureAtlas atlas(1024, 1024, 3);
	std::vector<glm::uvec2> lmPositions(lightmaps.size());
	for (auto i = 0u; i < lightmaps.size(); i++) {
		const auto& lm = lightmaps[i];
		if (lm.width == 0 || lm.height == 0)
			continue;
		lmPositions[i] = atlas.store(lm);
	}
	atlas.img().Save("lm_atlas.png");

	// recompute lightmap coords
	std::vector<std::vector<glm::vec2>> lmCoords(m_bsp->faces.size());
	for (const auto& face : m_bsp->faces) {
		const auto faceIndex = &face - &m_bsp->faces.front();
		auto& coords = m_bsp->faceTexCoords[faceIndex];
		for (auto& coord : coords.lightmapCoords)
			lmCoords[faceIndex].push_back(atlas.convertCoord(lightmaps[faceIndex], lmPositions[faceIndex], coord));
	}

	m_lightmapAtlas = m_renderer.createTexture({ atlas.img() });
	return lmCoords;
}

void BspRenderable::loadSkyTextures() {
	const auto images = m_bsp->loadSkyBox();
	if (!images)
		return;
	m_skyboxTex = m_renderer.createCubeTexture(*images);
}

void BspRenderable::render(const RenderSettings& settings) {
	m_settings = &settings;

	// render sky box
	if (m_skyboxTex && global::renderSkybox)
		renderSkybox();

	const auto& cameraPos = m_camera->position();

	if (global::renderStaticBSP || global::renderBrushEntities)
		for (auto&& b : facesDrawn)
			b = false;

	std::vector<render::EntityData> ents;
	if (global::renderStaticBSP)
		ents.push_back(render::EntityData{ renderStaticGeometry(cameraPos), glm::vec3{}, 1.0f, bsp30::RenderMode::RENDER_MODE_NORMAL });

	if (global::renderBrushEntities) {
		for (const auto i : m_bsp->brushEntities) {
			const auto& ent = m_bsp->entities[i];

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

			std::vector<render::FaceRenderInfo> fri;
			renderBSP(m_bsp->models[model].headNodesIndex[0], boost::dynamic_bitset<uint8_t>{}, cameraPos, fri); // for some odd reason, VIS does not work for entities ...

			ents.push_back(render::EntityData{ std::move(fri), m_bsp->models[model].origin, alpha, renderMode });
		}
	}

	m_renderer.renderStatic(std::move(ents), m_bsp->m_decals, *m_staticGeometryVao, *m_decalVao, m_textures, *m_lightmapAtlas, settings);

	// Leaf outlines
	if (global::renderLeafOutlines) {
		std::cerr << "Rendering leaf outlines is currently disabled\n";
		//glLoadMatrixf(glm::value_ptr(matrix));
		//renderLeafOutlines();
	}
}

void BspRenderable::renderSkybox() {
	// TODO: glm in WSL Ubuntu does not yet have this function
	auto matrix = m_settings->projection * glm::eulerAngleXZX(degToRad(m_settings->pitch - 90.0f), degToRad(-m_settings->yaw), degToRad(+90.0f));
	//auto matrix = m_settings->projection * glm::eulerAngleX(degToRad(m_settings->pitch - 90.0f)) * glm::eulerAngleZ(degToRad(-m_settings->yaw)) * glm::eulerAngleX(degToRad(+90.0f));

	m_renderer.renderSkyBox(**m_skyboxTex, matrix);
}

auto BspRenderable::renderStaticGeometry(glm::vec3 pos) -> std::vector<render::FaceRenderInfo> {
	std::vector<render::FaceRenderInfo> fri;
	const auto leaf = m_bsp->findLeaf(pos);
	renderBSP(0, !leaf || m_bsp->visLists.empty() ? boost::dynamic_bitset<std::uint8_t>{} : m_bsp->visLists[*leaf - 1], pos, fri);
	return fri;
}

//void BspRenderable::renderLeafOutlines() {
//	std::mt19937 engine;
//	std::uniform_real_distribution dist(0.0f, 1.0f);
//
//	glLineWidth(1.0f);
//	glLineStipple(1, 0xF0F0);
//	glEnable(GL_LINE_STIPPLE);
//	for (const auto& leaf : m_bsp->leaves) {
//		glColor3f(dist(engine), dist(engine), dist(engine));
//
//		glBegin(GL_LINES);
//		// Draw right face of bounding box
//		glVertex3f(leaf.upper[0], leaf.upper[1], leaf.upper[2]);
//		glVertex3f(leaf.upper[0], leaf.lower[1], leaf.upper[2]);
//		glVertex3f(leaf.upper[0], leaf.lower[1], leaf.upper[2]);
//		glVertex3f(leaf.upper[0], leaf.lower[1], leaf.lower[2]);
//		glVertex3f(leaf.upper[0], leaf.lower[1], leaf.lower[2]);
//		glVertex3f(leaf.upper[0], leaf.upper[1], leaf.lower[2]);
//		glVertex3f(leaf.upper[0], leaf.upper[1], leaf.lower[2]);
//		glVertex3f(leaf.upper[0], leaf.upper[1], leaf.upper[2]);
//
//		// Draw left face of bounding box
//		glVertex3f(leaf.lower[0], leaf.lower[1], leaf.lower[2]);
//		glVertex3f(leaf.lower[0], leaf.upper[1], leaf.lower[2]);
//		glVertex3f(leaf.lower[0], leaf.upper[1], leaf.lower[2]);
//		glVertex3f(leaf.lower[0], leaf.upper[1], leaf.upper[2]);
//		glVertex3f(leaf.lower[0], leaf.upper[1], leaf.upper[2]);
//		glVertex3f(leaf.lower[0], leaf.lower[1], leaf.upper[2]);
//		glVertex3f(leaf.lower[0], leaf.lower[1], leaf.upper[2]);
//		glVertex3f(leaf.lower[0], leaf.lower[1], leaf.lower[2]);
//
//		// Connect the faces
//		glVertex3f(leaf.lower[0], leaf.upper[1], leaf.upper[2]);
//		glVertex3f(leaf.upper[0], leaf.upper[1], leaf.upper[2]);
//		glVertex3f(leaf.lower[0], leaf.upper[1], leaf.lower[2]);
//		glVertex3f(leaf.upper[0], leaf.upper[1], leaf.lower[2]);
//		glVertex3f(leaf.lower[0], leaf.lower[1], leaf.lower[2]);
//		glVertex3f(leaf.upper[0], leaf.lower[1], leaf.lower[2]);
//		glVertex3f(leaf.lower[0], leaf.lower[1], leaf.upper[2]);
//		glVertex3f(leaf.upper[0], leaf.lower[1], leaf.upper[2]);
//		glEnd();
//	}
//	glDisable(GL_LINE_STIPPLE);
//	glColor3f(1, 1, 1);
//}

void BspRenderable::renderLeaf(int leaf, std::vector<render::FaceRenderInfo>& fris) {
	for (int i = 0; i < m_bsp->leaves[leaf].markSurfaceCount; i++) {
		const auto& faceIndex = m_bsp->markSurfaces[m_bsp->leaves[leaf].firstMarkSurface + i];

		if (facesDrawn[faceIndex])
			continue;
		facesDrawn[faceIndex] = true;

		const auto& face = m_bsp->faces[faceIndex];

		if (face.styles[0] == 0xFF)
			continue;

		// if the light map offset is not -1 and the lightmap lump is not empty, there are lightmaps
		const bool lightmapAvailable = static_cast<signed>(face.lightmapOffset) != -1 && m_bsp->header.lump[bsp30::LumpType::LUMP_LIGHTING].length > 0;

		auto& fri = fris.emplace_back();
		if (global::textures)
			fri.tex = m_textures[m_bsp->textureInfos[face.textureInfo].miptexIndex].get();
		else
			fri.tex = nullptr;

		fri.offset = vertexOffsets[faceIndex];
		fri.count = (face.edgeCount - 2) * 3;
	}
}

void BspRenderable::renderBSP(int node, const boost::dynamic_bitset<std::uint8_t>& visList, glm::vec3 pos, std::vector<render::FaceRenderInfo>& fri) {
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

void BspRenderable::buildBuffers(std::vector<std::vector<glm::vec2>>&& lmCoords) {
	{
		// static and brush geometry
		std::vector<VertexWithLM> vertices;

		for (const auto& face : m_bsp->faces) {
			const auto faceIndex = &face - &m_bsp->faces.front();
			const auto& coords = m_bsp->faceTexCoords[faceIndex];
			const auto firstIndex = vertices.size();
			for (int i = 0; i < face.edgeCount; i++) {
				if (i > 2) {
					auto first = vertices[firstIndex];
					auto prev = vertices.back();
					vertices.push_back(first);
					vertices.push_back(prev);
				}

				auto& v = vertices.emplace_back();
				v.texCoord = coords.texCoords[i];
				v.lightmapCoord = lmCoords[faceIndex].empty() ? glm::vec2{ 0.0 } : lmCoords[faceIndex][i];

				v.normal = m_bsp->planes[face.planeIndex].normal;
				if (face.planeSide)
					v.normal = -v.normal;

				int edge = m_bsp->surfEdges[face.firstEdgeIndex + i];
				if (edge > 0)
					v.position = m_bsp->vertices[m_bsp->edges[edge].vertexIndex[0]];
				else
					v.position = m_bsp->vertices[m_bsp->edges[-edge].vertexIndex[1]];
			}
			vertexOffsets.push_back(static_cast<unsigned int>(firstIndex));
		}

		m_staticGeometryVbo = m_renderer.createBuffer(vertices.size() * sizeof(VertexWithLM), vertices.data());
		m_staticGeometryVao = m_renderer.createInputLayout(*m_staticGeometryVbo, {
			render::AttributeLayout{ "POSITION", 0, 3, render::AttributeLayout::Type::Float, sizeof(VertexWithLM), offsetof(VertexWithLM, position     ) },
			render::AttributeLayout{ "NORMAL"  , 0, 3, render::AttributeLayout::Type::Float, sizeof(VertexWithLM), offsetof(VertexWithLM, normal       ) },
			render::AttributeLayout{ "TEXCOORD", 0, 2, render::AttributeLayout::Type::Float, sizeof(VertexWithLM), offsetof(VertexWithLM, texCoord     ) },
			render::AttributeLayout{ "TEXCOORD", 1, 2, render::AttributeLayout::Type::Float, sizeof(VertexWithLM), offsetof(VertexWithLM, lightmapCoord) }
		});
	}

	{
		// decals
		std::vector<Vertex> vertices;

		for (const auto& decal : m_bsp->m_decals) {
			for (auto i = 0; i < 6; i++) {
				auto& v = vertices.emplace_back();
				v.normal = decal.normal;
				if (i == 0 || i == 3) { v.position = decal.vec[0]; v.texCoord = glm::vec2(0, 0); }
				if (i == 1)           { v.position = decal.vec[1]; v.texCoord = glm::vec2(1, 0); }
				if (i == 2 || i == 4) { v.position = decal.vec[2]; v.texCoord = glm::vec2(1, 1); }
				if (i == 5)           { v.position = decal.vec[3]; v.texCoord = glm::vec2(0, 1); }
			}
		}

		m_decalVbo = m_renderer.createBuffer(vertices.size() * sizeof(Vertex), vertices.data());
		m_decalVao = m_renderer.createInputLayout(*m_decalVbo, {
			render::AttributeLayout{ "POSITION", 0, 3, render::AttributeLayout::Type::Float, sizeof(Vertex), offsetof(Vertex, position) },
			render::AttributeLayout{ "NORMAL"  , 0, 3, render::AttributeLayout::Type::Float, sizeof(Vertex), offsetof(Vertex, normal  ) },
			render::AttributeLayout{ "TEXCOORD", 0, 2, render::AttributeLayout::Type::Float, sizeof(Vertex), offsetof(Vertex, texCoord) },
			render::AttributeLayout{ "TEXCOORD", 1, 2, render::AttributeLayout::Type::Float, sizeof(Vertex), offsetof(Vertex, texCoord) }, // we do not need this one
		});
	}
}
