#include "GLRenderer.h"

#include <iostream>

#include "IPSS.h"
#include "IO.h"
#include "Bsp.h"
#include "Hud.h"
#include "font.h"

namespace {
	constexpr auto  FONT_HUD_HEIGHT = 12;
	constexpr auto  FONT_HUD_SPACE = 5;
	constexpr float FONT_HUD_COLOR[] = { 1.0f, 0.0f, 0.0f };

	constexpr auto CONSOLE_WIDTH = 400;
	constexpr auto CONSOLE_HEIGHT = 300;

	void glslShaderSourceFile(GLuint object, const fs::path& filename) {
		const auto source = readTextFile(filename);
		const auto p = source.c_str();
		glShaderSource(object, 1, &p, nullptr);
		std::clog << "Read shader from file " << filename << "\n";
	}

	void glslPrintProgramInfoLog(GLuint object) {
		GLint infologLength = 0;
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &infologLength);

		if (infologLength > 0) {
			std::string infoLog;
			infoLog.resize(infologLength);
			GLint charsWritten = 0;
			glGetProgramInfoLog(object, infologLength, &charsWritten, infoLog.data());
			if (infoLog[0] != 0 && infoLog != "")
				std::clog << infoLog << "\n";
			else
				std::clog << "(no program info log)\n";
		}
	}
}

GLRenderer::GLRenderer() {
	if (glewInit() != GLEW_OK)
		throw std::runtime_error("glew failed to initialize");

	// error callback
	if (GLEW_ARB_debug_output) {
		glDebugMessageCallbackARB([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
			std::cerr << "OpenGL debug callback: " << std::string(message, length);
		},
			nullptr);
	}

	std::clog << "Setting rendering states ...\n";
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	glEnable(GL_MULTISAMPLE);

	//
	// Extensions
	//

	std::clog << "Checking extensions ...\n";

	std::clog << "GL_ARB_multitexture ...";
	if (!GLEW_ARB_multitexture)
		throw std::runtime_error("GL_ARB_multitexture is not supported. Please upgrade your video driver.");
	std::clog << " OK\n";

	std::clog << "GL_ARB_texture_non_power_of_two ...";
	if (GLEW_ARB_texture_non_power_of_two)
		std::clog << " OK (no lightmap scaling needed)\n";
	else
		std::clog << " NOT SUPPORTED (lightmaps will be scaled to 16 x 16)\n";

	std::clog << "GLSL Shaders ...";
	if (!GLEW_ARB_shader_objects || !GLEW_ARB_shading_language_100 || !GL_ARB_vertex_shader || !GL_ARB_fragment_shader)
		throw std::runtime_error("GLSL shaders are not supported. Please upgrade your video driver.");
	std::clog << " OK\n";

	//
	// Shader
	//

	std::clog << "Loading shaders ...\n";
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

	//
	// configure lighting for flashlight
	//

	glEnable(GL_LIGHT0);

	GLfloat lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	GLfloat spotDir[] = { 0.0f, 0.0f, -1.0f };
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spotDir);
	glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 25.0f);
	glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 1.0f);
	glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.01f);
	glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.01f);
	glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0001f);

	// Load fonts
	std::clog << "Creating font ...\n";
#ifdef _WIN32
	m_font = createFont("System", FONT_HUD_HEIGHT);
#else
	m_font = createFont("helvetica", FONT_HUD_HEIGHT);
#endif
}

void GLRenderer::resizeViewport(int width, int height)
{
	glViewport(0, 0, width, height);
	m_projectionMatrix = glm::perspective(degToRad(60.0f), static_cast<GLfloat>(width) / static_cast<GLfloat>(height), 8.0f, 4000.0f);
}

void GLRenderer::beginFrame(RenderSettings settings, glm::mat4 viewMatrix) {
	m_settings = settings;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

	// Enable Shader
	glUseProgram(m_shaderProgram);

	glUniform1i(glGetUniformLocation(m_shaderProgram, "tex1"), 0);
	glUniform1i(glGetUniformLocation(m_shaderProgram, "tex2"), 1);
	glUniform1i(glGetUniformLocation(m_shaderProgram, "nightvision"), static_cast<GLint>(m_settings.nightvision));
	glUniform1i(glGetUniformLocation(m_shaderProgram, "flashlight"), static_cast<GLint>(m_settings.flashlight));

	const auto matrix = m_projectionMatrix * viewMatrix;
	glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "matrix"), 1, false, glm::value_ptr(matrix));
}

void GLRenderer::render(const Bsp& bsp, const glm::vec3 cameraPos) {
	// render sky box
	if (bsp.hasSkyBox() && m_settings.renderSkybox) {
		glUniform1i(glGetUniformLocation(m_shaderProgram, "unit1Enabled"), 1);
		renderSkyBox(bsp, cameraPos);
		glUniform1i(glGetUniformLocation(m_shaderProgram, "unit1Enabled"), 0);
	}

	// turn on needed texture units
	glUniform1i(glGetUniformLocation(m_shaderProgram, "unit1Enabled"), static_cast<GLint>(m_settings.textures || m_settings.lightmaps));
	glUniform1i(glGetUniformLocation(m_shaderProgram, "unit2Enabled"), static_cast<GLint>(m_settings.textures && m_settings.lightmaps));

	glEnable(GL_DEPTH_TEST);

	if (m_settings.renderStaticBSP)
		renderStaticGeometry(bsp, cameraPos);

	if (m_settings.renderBrushEntities)
		renderBrushEntities(bsp, cameraPos);

	// Turn off second unit, if it was enabled
	if (m_settings.useShader)
		glUniform1i(glGetUniformLocation(m_shaderProgram, "unit2Enabled"), 0);
	else {
		if (m_settings.lightmaps && m_settings.textures) {
			glActiveTexture(GL_TEXTURE1_ARB);
			glDisable(GL_TEXTURE_2D);
		}
	}

	if (m_settings.renderDecals) {
		glActiveTexture(GL_TEXTURE0_ARB);
		renderDecals(bsp);
	}

	// Turn off first unit, if it was enabled
	if (m_settings.useShader)
		glUniform1i(glGetUniformLocation(m_shaderProgram, "unit1Enabled"), 0);
	else {
		if (m_settings.lightmaps || m_settings.textures) {
			glActiveTexture(GL_TEXTURE0_ARB);
			glDisable(GL_TEXTURE_2D);
		}
	}

	glDisable(GL_DEPTH_TEST);

	if (m_settings.useShader)
		glUseProgram(0);

	// Leaf outlines
	if (m_settings.renderLeafOutlines)
		renderLeafOutlines(bsp);
}

void GLRenderer::renderHud(const Hud& hud, unsigned int width, unsigned int height, glm::vec3 cameraPos, glm::vec2 cameraAngles, glm::vec3 cameraView, double fps) {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glOrtho(0, width, 0, height, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	int nCurrentY = height;

	glColor3fv(FONT_HUD_COLOR);
	glPuts(FONT_HUD_SPACE, nCurrentY -= (FONT_HUD_SPACE + FONT_HUD_HEIGHT), m_font,
		IPSS() << std::fixed << std::setprecision(1) << "FPS: " << fps);
	glPuts(FONT_HUD_SPACE, nCurrentY -= (FONT_HUD_SPACE + FONT_HUD_HEIGHT), m_font,
		IPSS() << std::fixed << std::setprecision(1) << "Cam pos: " << cameraPos.x << "x " << cameraPos.y << "y " << cameraPos.z << "z");
	glPuts(FONT_HUD_SPACE, nCurrentY -= (FONT_HUD_SPACE + FONT_HUD_HEIGHT), m_font,
		IPSS() << std::fixed << std::setprecision(1) << "Cam view: " << cameraAngles.x << "°pitch " << cameraAngles.y << "°yaw (vec: " << cameraView.x << "x " << cameraView.y << "y " << cameraView.z << "z)");

	// console
	nCurrentY = FONT_HUD_SPACE;
	for (const auto& line : hud.console()) {
		if (nCurrentY + FONT_HUD_HEIGHT >= CONSOLE_HEIGHT)
			break;
		glPuts(FONT_HUD_SPACE, nCurrentY, m_font, line);
		nCurrentY += FONT_HUD_HEIGHT + FONT_HUD_SPACE;
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void GLRenderer::renderCoords() {
	glLineWidth(3.0f);
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f); //red X+
	glVertex3i(4000, 0, 0);
	glVertex3i(0, 0, 0);
	glColor3f(0.0f, 1.0f, 0.0f); //green Y+
	glVertex3i(0, 4000, 0);
	glVertex3i(0, 0, 0);
	glColor3f(0.0f, 0.0f, 1.0f); //blue Z+
	glVertex3i(0, 0, 4000);
	glVertex3i(0, 0, 0);
	glEnd();

	glLineWidth(1.0f);
	glBegin(GL_LINES);
	glColor3f(0.0f, 0.4f, 0.0f); //green Y-
	glVertex3i(0, 0, 0);
	glVertex3i(0, -4000, 0);
	glColor3f(0.4f, 0.0f, 0.0f); //red X-
	glVertex3i(0, 0, 0);
	glVertex3i(-4000, 0, 0);
	glColor3f(0.0f, 0.0f, 0.4f); //blue Z-
	glVertex3i(0, 0, 0);
	glVertex3i(0, 0, -4000);
	glEnd();
}

void GLRenderer::renderSkyBox(const Bsp& bsp, const glm::vec3 cameraPos) {
	glPushMatrix();
	glTranslatef(cameraPos.x, cameraPos.y, cameraPos.z);
	glCallList(*bsp.skyBoxDL);
	glPopMatrix();
}

void GLRenderer::renderStaticGeometry(const Bsp& bsp, vec3 vPos) {
	for (auto&& b : bsp.facesDrawn)
		b = false;

	const int iLeaf = bsp.findLeaf(vPos); //Get the leaf where the camera is in
	renderBSP(bsp, 0, iLeaf, vPos);
}

void GLRenderer::renderBrushEntities(const Bsp& bsp, vec3 vPos) {
	for (int i = 0; i < bsp.brushEntities.size(); i++) // TODO(bernh): implement PVS for pEntities
		renderBrushEntity(bsp, i, vPos);
}

void GLRenderer::renderDecals(const Bsp& bsp) {
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.0f, -2.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (auto& decal : bsp.decals) {
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


void GLRenderer::renderLeafOutlines(const Bsp& bsp)
{
	glLineWidth(1.0f);
	glLineStipple(1, 0xF0F0);
	glEnable(GL_LINE_STIPPLE);
	for (const auto& leaf : bsp.leaves)
		renderLeafOutlines(leaf);
	glDisable(GL_LINE_STIPPLE);
	glColor3f(1, 1, 1);
}

void GLRenderer::renderLeafOutlines(const bsp30::Leaf& leaf) {
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


void GLRenderer::renderFace(const Bsp& bsp, int face) {
	if (bsp.facesDrawn[face])
		return;
	bsp.facesDrawn[face] = true;

	if (bsp.faces[face].styles[0] == 0xFF)
		return;

	auto renderTriangle = [&](int i) {
		vec3 normal = bsp.planes[bsp.faces[face].planeIndex].normal;
		if (bsp.faces[face].planeSide)
			normal = -normal;
		glNormal3f(normal.x, normal.y, normal.z);

		int edge = bsp.surfEdges[bsp.faces[face].firstEdgeIndex + i];
		if (edge > 0)
			glVertex3f(bsp.vertices[bsp.edges[edge].vertexIndex[0]].x, bsp.vertices[bsp.edges[edge].vertexIndex[0]].y, bsp.vertices[bsp.edges[edge].vertexIndex[0]].z);
		else {
			edge *= -1;
			glVertex3f(bsp.vertices[bsp.edges[edge].vertexIndex[1]].x, bsp.vertices[bsp.edges[edge].vertexIndex[1]].y, bsp.vertices[bsp.edges[edge].vertexIndex[1]].z);
		}
	};

	// if the light map offset is not -1 and the lightmap lump is not empty, there are lightmaps
	bool bLightmapAvail = static_cast<signed>(bsp.faces[face].lightmapOffset) != -1 && bsp.header.lump[bsp30::LumpType::LUMP_LIGHTING].length > 0;

	if (bLightmapAvail && m_settings.lightmaps && m_settings.textures) {
		// We need both texture units for textures and lightmaps

		// base texture
		glActiveTexture(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_2D, bsp.textureIds[bsp.textureInfos[bsp.faces[face].textureInfo].miptexIndex]);

		// light map
		glActiveTexture(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, bsp.lightmapTexIds[face]);

		glBegin(GL_TRIANGLE_FAN);
		for (int i = 0; i < bsp.faces[face].edgeCount; i++) {
			glMultiTexCoord2f(GL_TEXTURE0_ARB, bsp.faceTexCoords[face].texCoords[i].s, bsp.faceTexCoords[face].texCoords[i].t);
			glMultiTexCoord2f(GL_TEXTURE1_ARB, bsp.faceTexCoords[face].lightmapCoords[i].s, bsp.faceTexCoords[face].lightmapCoords[i].t);
			renderTriangle(i);
		}
		glEnd();
	}
	else {
		// We need one texture unit for either textures or lightmaps
		glActiveTexture(GL_TEXTURE0_ARB);

		if (m_settings.lightmaps)
			glBindTexture(GL_TEXTURE_2D, bsp.lightmapTexIds[face]);
		else
			glBindTexture(GL_TEXTURE_2D, bsp.textureIds[bsp.textureInfos[bsp.faces[face].textureInfo].miptexIndex]);

		glBegin(GL_TRIANGLE_FAN);
		for (int i = 0; i < bsp.faces[face].edgeCount; i++) {
			if (m_settings.lightmaps)
				glTexCoord2f(bsp.faceTexCoords[face].lightmapCoords[i].s, bsp.faceTexCoords[face].lightmapCoords[i].t);
			else
				glTexCoord2f(bsp.faceTexCoords[face].texCoords[i].s, bsp.faceTexCoords[face].texCoords[i].t);
			renderTriangle(i);
		}
		glEnd();
	}
}

void GLRenderer::renderLeaf(const Bsp& bsp, int leaf) {
	for (int i = 0; i < bsp.leaves[leaf].markSurfaceCount; i++)
		renderFace(bsp, bsp.markSurfaces[bsp.leaves[leaf].firstMarkSurface + i]);
}

void GLRenderer::renderBSP(const Bsp& bsp, int iNode, int iCurrentLeaf, vec3 vPos) {
	if (iNode < 0) {
		if (iNode == -1)
			return;

		if (iCurrentLeaf > 0)
			if (!bsp.visLists.empty() && !bsp.visLists[iCurrentLeaf - 1].empty() && !bsp.visLists[iCurrentLeaf - 1][~iNode - 1])
				return;

		renderLeaf(bsp, ~iNode);

		return;
	}

	const auto dist = [&] {
		switch (bsp.planes[bsp.nodes[iNode].planeIndex].type) {
		case bsp30::PLANE_X: return vPos.x - bsp.planes[bsp.nodes[iNode].planeIndex].dist;
		case bsp30::PLANE_Y: return vPos.y - bsp.planes[bsp.nodes[iNode].planeIndex].dist;
		case bsp30::PLANE_Z: return vPos.z - bsp.planes[bsp.nodes[iNode].planeIndex].dist;
		default:             return glm::dot(bsp.planes[bsp.nodes[iNode].planeIndex].normal, vPos) - bsp.planes[bsp.nodes[iNode].planeIndex].dist;
		}
	}();

	const auto child1 = dist > 0 ? 1 : 0;
	const auto child2 = dist > 0 ? 0 : 1;
	renderBSP(bsp, bsp.nodes[iNode].childIndex[child1], iCurrentLeaf, vPos);
	renderBSP(bsp, bsp.nodes[iNode].childIndex[child2], iCurrentLeaf, vPos);
}

void GLRenderer::renderBrushEntity(const Bsp& bsp, int iEntity, vec3 vPos) {
	const auto& ent = bsp.entities[bsp.brushEntities[iEntity]];

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

	glPushMatrix();
	glTranslatef(bsp.models[iModel].vOrigin.x, bsp.models[iModel].vOrigin.y, bsp.models[iModel].vOrigin.z);

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

	renderBSP(bsp, bsp.models[iModel].headNodesIndex[0], -1, vPos);

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

	glPopMatrix();
}
