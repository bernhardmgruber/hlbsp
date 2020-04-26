#include "Window.h"

#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>

#include "Bsp.h"
#include "BspRenderable.h"
#include "HudRenderable.h"
#include "Image.h"
#include "global.h"

namespace {
	constexpr auto WINDOW_CAPTION = "HL BSP";

	constexpr auto cl_sidespeed = 400.0f;
	constexpr auto cl_forwardspeed = 400.0f;
	constexpr auto cl_downspeed = 400.0f;
	constexpr auto cl_pitchup = 89.0f;
	constexpr auto cl_pitchdown = 89.0f;

	constexpr auto m_yaw = 0.022f;
	constexpr auto m_pitch = 0.022f;
	constexpr auto sensitivity = 5.0f;
}

Window::Window(render::IPlatform& platform, Bsp& bsp)
	: GlfwWindow(WINDOW_CAPTION, platform)
	, hud(camera, timer)
	, bsp(bsp)
	, m_platform(platform) {

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_Init(handle(), false, GlfwClientApi_Unknown);

	m_renderer = platform.createRenderer();

	m_renderables.emplace_back(std::make_unique<BspRenderable>(*m_renderer, bsp, camera));
	m_renderables.emplace_back(std::make_unique<HudRenderable>(*m_renderer, hud));

	onResize(m_width, m_height);

	// place camera at spawn
	if (const auto info_player_start = bsp.FindEntity("info_player_start")) {
		if (auto origin = info_player_start->findProperty("origin")) {
			auto& o = camera.position();
			std::istringstream(*origin) >> o.x >> o.y >> o.z;
		}

		if (auto angle = info_player_start->findProperty("angle")) {
			camera.yaw() = std::stof(*angle);
		}
	}
}

Window::~Window() {
	m_renderer = nullptr;
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

auto Window::createMove() -> UserCommand {
	UserCommand cmd{};
	if (glfwGetKey(handle(), GLFW_KEY_SPACE) == GLFW_PRESS) {
		//cmd.upmove += cl_downspeed;
		cmd.buttons |= IN_JUMP;
	}
	//if (glfwGetKey(handle(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
	//	cmd.upmove -= cl_downspeed;
	if (glfwGetKey(handle(), GLFW_KEY_W) == GLFW_PRESS)
		cmd.forwardmove += cl_forwardspeed;
	if (glfwGetKey(handle(), GLFW_KEY_S) == GLFW_PRESS)
		cmd.forwardmove -= cl_forwardspeed;
	if (glfwGetKey(handle(), GLFW_KEY_A) == GLFW_PRESS)
		cmd.sidemove -= cl_sidespeed;
	if (glfwGetKey(handle(), GLFW_KEY_D) == GLFW_PRESS)
		cmd.sidemove += cl_sidespeed;

	return cmd;
}

void Window::mouseMove(UserCommand& cmd) {
	glm::dvec2 delta{0, 0};
	if (m_captureMouse) {
		glm::dvec2 pos;
		glfwGetCursorPos(handle(), &pos.x, &pos.y);
		delta = m_mouseDownPos - pos;
		glfwSetCursorPos(handle(), m_mouseDownPos.x, m_mouseDownPos.y);
	}

	auto yaw = camera.yaw();
	auto pitch = camera.pitch();
	yaw += m_yaw * delta.x * sensitivity;
	pitch += m_pitch * -delta.y * sensitivity; // pitch in halflife is negative when looking upward. cf: https://www.jwchong.com/hl/player.html
	if (pitch > cl_pitchdown)
		pitch = cl_pitchdown;
	if (pitch < -cl_pitchup)
		pitch = -cl_pitchup;

	cmd.viewangles.x = pitch;
	cmd.viewangles.y = yaw;
}

void Window::update() {
	timer.Tick();

	std::stringstream windowText;
	windowText << WINDOW_CAPTION << " - " << std::setprecision(1) << std::fixed << timer.TPS << " FPS";
	glfwSetWindowTitle(handle(), windowText.str().c_str());

	auto cmd = createMove();
	mouseMove(cmd);
	cmd.frameTime = timer.interval;
	camera.update(bsp.hulls[global::hullIndex], cmd);
}

void Window::draw() {
	ImGui_ImplGlfw_NewFrame();

	m_settings.projection = camera.projectionMatrix();
	m_settings.view = camera.viewMatrix();
	m_settings.pitch = camera.pitch();
	m_settings.yaw = camera.yaw();

	m_renderer->clear();
	for (auto& renderable : m_renderables)
		renderable->render(m_settings);
	if (global::renderCoords)
		m_renderer->renderCoords(m_settings.projection * m_settings.view);
}

void Window::onResize(int width, int height) {
	if (height == 0)
		height = 1;

	std::clog << "Window dimensions changed to " << width << " x " << height << "\n";

	camera.viewportWidth = width;
	camera.viewportHeight = height;

	m_renderer->resizeViewport(width, height);
}

void Window::onMouseButton(int button, int action, int modifiers) {
	ImGui_ImplGlfw_MouseButtonCallback(handle(), button, action, modifiers);

	if (action == GLFW_PRESS) {
		switch (button) {
			case GLFW_MOUSE_BUTTON_RIGHT:
				m_captureMouse = true;
				glfwGetCursorPos(handle(), &m_mouseDownPos.x, &m_mouseDownPos.y);
				glfwSetInputMode(handle(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
				break;
		}
	}

	if (action == GLFW_RELEASE) {
		switch (button) {
			case GLFW_MOUSE_BUTTON_RIGHT:
				m_captureMouse = false;
				glfwSetInputMode(handle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				break;
		}
	}
}

void Window::onMouseMove(double xOffset, double yOffset) {}

void Window::onMouseWheel(double xOffset, double yOffset) {
	ImGui_ImplGlfw_ScrollCallback(handle(), xOffset, yOffset);
}

void Window::onKey(int key, int scancode, int action, int mods) {
	ImGui_ImplGlfw_KeyCallback(handle(), key, scancode, action, mods);

	if (action == GLFW_PRESS) {
		switch (key) {
			//case GLFW_KEY_TAB:
			//	camera.moveSensitivity = CAMERA_MOVE_SENS * 3.0f;
			//	break;
			//case GLFW_KEY_LEFT_SHIFT:
			//	camera.moveSensitivity = CAMERA_MOVE_SENS / 3.0f;
			//	break;
			case GLFW_KEY_F1:
				if (m_fullscreen)
					std::clog << "Changing to windowed mode ...\n";
				else
					std::clog << "Changing to fullscreen ...\n";
				toggleFullscreen();
				break;

			case GLFW_KEY_F5: {
				const auto img = m_renderer->screenshot();

				std::string filename;
				for (auto i = 1;; i++) 
					if (!fs::exists(filename = "screenshots/Screenshot" + std::to_string(i) + ".bmp"))
						break;

				img.Save(filename);
				break;
			}

			case GLFW_KEY_C:
				global::renderCoords = !global::renderCoords;
				if (global::renderCoords)
					hud.print("coords enabled");
				else
					hud.print("coords disabled");
				break;

			case GLFW_KEY_H:
				global::renderHUD = !global::renderHUD;
				if (global::renderHUD)
					hud.print("hud enabled");
				else
					hud.print("hud disabled");
				break;

			case GLFW_KEY_L:
				global::lightmaps = !global::lightmaps;
				if (global::lightmaps)
					hud.print("lightmaps enabled");
				else
					hud.print("lightmaps disabled");
				break;

			case GLFW_KEY_N:
				global::nightvision = !global::nightvision;
				if (global::nightvision)
					hud.print("nightvision enabled");
				else
					hud.print("nightvision disabled");
				break;

			case GLFW_KEY_T:
				global::textures = !global::textures;
				if (global::textures)
					hud.print("textures enabled");
				else
					hud.print("textures disabled");
				break;

			case GLFW_KEY_O:
				global::renderLeafOutlines = !global::renderLeafOutlines;
				if (global::renderLeafOutlines)
					hud.print("leaf outlines enabled");
				else
					hud.print("leaf outlines disabled");
				break;

			//case GLFW_KEY_P:
			//	global::polygons = !global::polygons;
			//	if (global::polygons) {
			//		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			//		hud.print("polygon mode set to line");
			//	} else {
			//		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			//		hud.print("polygon mode set to fill");
			//	}
			//	break;

			case GLFW_KEY_V:
				global::flashlight = !global::flashlight;
				if (global::flashlight)
					hud.print("flashlight enabled");
				else
					hud.print("flashlight disabled");
				break;

			case GLFW_KEY_1:
				global::renderSkybox = !global::renderSkybox;
				if (global::renderSkybox)
					hud.print("skybox enabled");
				else
					hud.print("skybox disabled");
				break;

			case GLFW_KEY_2:
				global::renderStaticBSP = !global::renderStaticBSP;
				if (global::renderStaticBSP)
					hud.print("static geometry enabled");
				else
					hud.print("static geometry  disabled");
				break;

			case GLFW_KEY_3:
				global::renderBrushEntities = !global::renderBrushEntities;
				if (global::renderBrushEntities)
					hud.print("entities enabled");
				else
					hud.print("entities disabled");
				break;

			case GLFW_KEY_4:
				global::renderDecals = !global::renderDecals;
				if (global::renderDecals)
					hud.print("decals enabled");
				else
					hud.print("decals disabled");
				break;
		}
	}

	//if (action == GLFW_RELEASE) {
	//	switch (key) {
	//		case GLFW_KEY_TAB:
	//		case GLFW_KEY_LEFT_SHIFT:
	//			camera.moveSensitivity = CAMERA_MOVE_SENS;
	//			break;
	//	}
	//	return;
	//}
}

void Window::onChar(unsigned int codepoint) {
	ImGui_ImplGlfw_CharCallback(handle(), codepoint);
}
