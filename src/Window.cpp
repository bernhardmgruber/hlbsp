#include "Window.h"

#include <iostream>

#include "Image.h"
#include "BspRenderable.h"

namespace {
	const fs::path BSP_DIR       = "../../data/maps";
	const fs::path BSP_FILE_NAME = "cs_assault.bsp";

	constexpr auto WINDOW_CAPTION = "HL BSP";
}

Window::Window()
	: GlfwWindow(WINDOW_CAPTION)
	, bsp(BSP_DIR / BSP_FILE_NAME , m_settings.textures, m_settings.lightmaps) {

	m_renderer.addRenderable(std::make_unique<BspRenderable>(bsp, camera));

	onResize(m_width, m_height);

	// place camera at spawn
	if (const auto info_player_start = bsp.FindEntity("info_player_start")) {
		if (auto origin = info_player_start->findProperty("origin")) {
			std::istringstream iss(*origin);
			vec3               o;
			iss >> o.x >> o.y >> o.z;
			camera.setPosition(o);
		}

		if (auto angle = info_player_start->findProperty("angle")) {
			std::istringstream iss(*angle);
			float              yaw;
			iss >> yaw;
			camera.setViewAngles({0.0f, yaw});
		}
	}
}

Window::~Window() = default;

void Window::update() {
	timer.Tick();

	std::stringstream windowText;
	windowText << WINDOW_CAPTION << " - " << std::setprecision(1) << std::fixed << timer.TPS << " FPS";
	glfwSetWindowTitle(handle(), windowText.str().c_str());

	// camera
	glm::dvec2 delta{0, 0};
	if (m_captureMouse) {
		glm::dvec2 pos;
		glfwGetCursorPos(handle(), &pos.x, &pos.y);
		delta = m_mouseDownPos - pos;
		glfwSetCursorPos(handle(), m_mouseDownPos.x, m_mouseDownPos.y);
	}

	uint8_t moveFlags = 0;
	if (glfwGetKey(handle(), GLFW_KEY_SPACE) == GLFW_PRESS) moveFlags |= Up;
	if (glfwGetKey(handle(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) moveFlags |= Down;
	if (glfwGetKey(handle(), GLFW_KEY_W) == GLFW_PRESS) moveFlags |= Forward;
	if (glfwGetKey(handle(), GLFW_KEY_S) == GLFW_PRESS) moveFlags |= Backward;
	if (glfwGetKey(handle(), GLFW_KEY_A) == GLFW_PRESS) moveFlags |= Left;
	if (glfwGetKey(handle(), GLFW_KEY_D) == GLFW_PRESS) moveFlags |= Right;

	camera.update(timer.interval, delta.x, delta.y, moveFlags);
}

void Window::draw() {
	m_renderer.beginFrame(m_settings, camera.viewMatrix());
	m_renderer.render();

	if (m_settings.renderCoords)
		m_renderer.renderCoords();

	if (m_settings.renderHUD)
		m_renderer.renderHud(hud, m_width, m_height, camera.position(), camera.viewAngles(), camera.viewVector(), timer.TPS);
}

void Window::onResize(int width, int height) {
	if (height == 0)
		height = 1;

	m_width = width;
	m_height = height;
	std::clog << "Window dimensions changed to " << width << " x " << height << "\n";

	m_renderer.resizeViewport(width, height);
}

void Window::onMouseButton(int button, int action, int modifiers) {
	if (action == GLFW_PRESS) {
		switch (button) {
			case GLFW_MOUSE_BUTTON_RIGHT:
				m_captureMouse = true;
				glfwGetCursorPos(handle(), &m_mouseDownPos.x, &m_mouseDownPos.y);
				//glfwSetInputMode(handle(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
				break;
		}
	}

	if (action == GLFW_RELEASE) {
		switch (button) {
			case GLFW_MOUSE_BUTTON_RIGHT:
				m_captureMouse = false;
				//glfwSetInputMode(handle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				break;
		}
	}
}

void Window::onMouseMove(double dx, double dy) {}

void Window::onMouseWheel(double xOffset, double yOffset) {}

void Window::onKey(int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		switch (key) {
			case GLFW_KEY_TAB:
				camera.setMoveSens(CAMERA_MOVE_SENS * 3.0f);
				break;
			case GLFW_KEY_LEFT_SHIFT:
				camera.setMoveSens(CAMERA_MOVE_SENS / 3.0f);
				break;
			case GLFW_KEY_F1:
				if (m_fullscreen)
					std::clog << "Changing to windowed mode ...\n";
				else
					std::clog << "Changing to fullscreen ...\n";
				toggleFullscreen();
				break;

			case GLFW_KEY_F5: {
				Image pImg(m_width, m_height, 3);
				glReadPixels(0, 0, pImg.width, pImg.height, GL_RGB, GL_UNSIGNED_BYTE, pImg.data.data());

				//get filename
				char szFileName[512];

				for (int i = 1;; i++) {
					sprintf(szFileName, "screenshots/Screenshot%d.bmp", i);
					FILE* pFile;
					if ((pFile = fopen(szFileName, "rbe")) == nullptr)
						break;
					fclose(pFile);
				}

				pImg.Save(szFileName);
				break;
			}

			case GLFW_KEY_C:
				m_settings.renderCoords = !m_settings.renderCoords;
				if (m_settings.renderCoords)
					hud.print("coords enabled");
				else
					hud.print("coords disabled");
				break;

			case GLFW_KEY_H:
				m_settings.renderHUD = !m_settings.renderHUD;
				if (m_settings.renderHUD)
					hud.print("hud enabled");
				else
					hud.print("hud disabled");
				break;

			case GLFW_KEY_L:
				m_settings.lightmaps = !m_settings.lightmaps;
				if (m_settings.lightmaps)
					hud.print("lightmaps enabled");
				else
					hud.print("lightmaps disabled");
				break;

			case GLFW_KEY_N:
				m_settings.nightvision = !m_settings.nightvision;
				if (m_settings.nightvision)
					hud.print("nightvision enabled");
				else
					hud.print("nightvision disabled");
				break;

			case GLFW_KEY_T:
				m_settings.textures = !m_settings.textures;
				if (m_settings.textures)
					hud.print("textures enabled");
				else
					hud.print("textures disabled");
				break;

			case GLFW_KEY_O:
				m_settings.renderLeafOutlines = !m_settings.renderLeafOutlines;
				if (m_settings.renderLeafOutlines)
					hud.print("leaf outlines enabled");
				else
					hud.print("leaf outlines disabled");
				break;

			case GLFW_KEY_P:
				m_settings.polygons = !m_settings.polygons;
				if (m_settings.polygons) {
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					hud.print("polygon mode set to line");
				} else {
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					hud.print("polygon mode set to fill");
				}
				break;

			case GLFW_KEY_V:
				m_settings.flashlight = !m_settings.flashlight;
				if (m_settings.flashlight)
					hud.print("flashlight enabled");
				else
					hud.print("flashlight disabled");
				break;

			case GLFW_KEY_1:
				m_settings.renderSkybox = !m_settings.renderSkybox;
				if (m_settings.renderSkybox)
					hud.print("skybox enabled");
				else
					hud.print("skybox disabled");
				break;

			case GLFW_KEY_2:
				m_settings.renderStaticBSP = !m_settings.renderStaticBSP;
				if (m_settings.renderStaticBSP)
					hud.print("static geometry enabled");
				else
					hud.print("static geometry  disabled");
				break;

			case GLFW_KEY_3:
				m_settings.renderBrushEntities = !m_settings.renderBrushEntities;
				if (m_settings.renderBrushEntities)
					hud.print("entities enabled");
				else
					hud.print("entities disabled");
				break;

			case GLFW_KEY_4:
				m_settings.renderDecals = !m_settings.renderDecals;
				if (m_settings.renderDecals)
					hud.print("decals enabled");
				else
					hud.print("decals disabled");
				break;
		}
	}

	if (action == GLFW_RELEASE) {
		switch (key) {
			case GLFW_KEY_TAB:
			case GLFW_KEY_LEFT_SHIFT:
				camera.setMoveSens(CAMERA_MOVE_SENS);
				break;
		}
		return;
	}
}

void Window::onChar(unsigned int codepoint) {}
