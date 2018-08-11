#include "GLRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

#include "IPSS.h"
#include "IO.h"
#include "Hud.h"
#include "font.h"
#include "mathlib.h"

namespace {
	constexpr auto  FONT_HUD_HEIGHT = 12;
	constexpr auto  FONT_HUD_SPACE = 5;
	constexpr float FONT_HUD_COLOR[] = { 1.0f, 0.0f, 0.0f };

	constexpr auto CONSOLE_WIDTH = 400;
	constexpr auto CONSOLE_HEIGHT = 300;
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

void GLRenderer::addRenderable(std::unique_ptr<IRenderable> renderable)
{
	m_renderables.emplace_back(std::move(renderable));
}

void GLRenderer::resizeViewport(int width, int height)
{
	glViewport(0, 0, width, height);
	m_projectionMatrix = glm::perspective(degToRad(60.0f), static_cast<GLfloat>(width) / static_cast<GLfloat>(height), 8.0f, 4000.0f);
}

void GLRenderer::beginFrame(RenderSettings settings, glm::mat4 viewMatrix) {
	m_settings = settings;
	m_settings.matrix = m_projectionMatrix * viewMatrix;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
}

void GLRenderer::render() {
	for (auto& renderable : m_renderables)
		renderable->render(m_settings);
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
