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

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(60.0f, static_cast<GLfloat>(width) / static_cast<GLfloat>(height), 8.0f, 4000.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void GLRenderer::beginFrame(RenderSettings settings) {
	m_settings = settings;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

	// Enable Shader
	glUseProgram(m_shaderProgram);

	glUniform1i(glGetUniformLocation(m_shaderProgram, "tex1"), 0);
	glUniform1i(glGetUniformLocation(m_shaderProgram, "tex2"), 1);
	glUniform1i(glGetUniformLocation(m_shaderProgram, "bNightvision"), static_cast<GLint>(m_settings.nightvision));
	glUniform1i(glGetUniformLocation(m_shaderProgram, "bFlashlight"), static_cast<GLint>(m_settings.flashlight));
}

void GLRenderer::render(const Bsp& bsp, const glm::vec3 cameraPos) {
	// render sky box
	if (bsp.hasSkyBox() && m_settings.renderSkybox) {
		glUniform1i(glGetUniformLocation(m_shaderProgram, "bUnit1Enabled"), 1);
		renderSkyBox(bsp, cameraPos);
		glUniform1i(glGetUniformLocation(m_shaderProgram, "bUnit1Enabled"), 0);
	}

	// turn on needed texture units
	glUniform1i(glGetUniformLocation(m_shaderProgram, "bUnit1Enabled"), static_cast<GLint>(m_settings.textures || m_settings.lightmaps));
	glUniform1i(glGetUniformLocation(m_shaderProgram, "bUnit2Enabled"), static_cast<GLint>(m_settings.textures && m_settings.lightmaps));

	glEnable(GL_DEPTH_TEST);

	if (m_settings.renderStaticBSP)
		bsp.RenderStaticGeometry(cameraPos);

	if (m_settings.renderBrushEntities)
		bsp.RenderBrushEntities(cameraPos);

	// Turn off second unit, if it was enabled
	if (m_settings.useShader)
		glUniform1i(glGetUniformLocation(m_shaderProgram, "bUnit2Enabled"), 0);
	else {
		if (m_settings.lightmaps && m_settings.textures) {
			glActiveTexture(GL_TEXTURE1_ARB);
			glDisable(GL_TEXTURE_2D);
		}
	}

	if (m_settings.renderDecals) {
		glActiveTexture(GL_TEXTURE0_ARB);
		bsp.RenderDecals();
	}

	// Turn off first unit, if it was enabled
	if (m_settings.useShader)
		glUniform1i(glGetUniformLocation(m_shaderProgram, "bUnit1Enabled"), 0);
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
		bsp.RenderLeavesOutlines();
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

void GLRenderer::renderSkyBox(const Bsp& bsp, const glm::vec3 cameraPos)
{
	glPushMatrix();
	glTranslatef(cameraPos.x, cameraPos.y, cameraPos.z);
	glCallList(*bsp.skyBoxDL);
	glPopMatrix();
}
