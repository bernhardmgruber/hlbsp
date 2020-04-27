#include "Hud.h"

#include <imgui.h>

#include "Camera.h"
#include "IPSS.h"
#include "Timer.h"
#include "global.h"

namespace {
	constexpr auto FONT_HUD_HEIGHT = 12;
	const glm::vec3 FONT_HUD_COLOR = {1.0f, 0.0f, 0.0f};
}

Hud::Hud(const Camera& camera, const Timer& timer)
	: m_camera(camera), m_timer(timer) {}

void Hud::print(std::string text) {
	m_console.push_back(std::move(text));
}

auto Hud::drawData() const -> ImDrawData* {
	const auto& cameraPos = m_camera.position();
	const auto& cameraView = m_camera.viewVector();
	const auto& pitch = m_camera.pitch();
	const auto& yaw = m_camera.yaw();
	const auto& fps = m_timer.TPS;

	ImGui::NewFrame();

	ImGui::Begin("Camera");
	ImGui::LabelText("FPS", (IPSS() << std::fixed << std::setprecision(1) << fps).str().c_str());
	ImGui::LabelText("cam pos", (IPSS() << std::fixed << std::setprecision(1) << cameraPos.x << "x " << cameraPos.y << "y " << cameraPos.z << "z").str().c_str());
	ImGui::LabelText("cam view", (IPSS() << std::fixed << std::setprecision(1) << pitch << " pitch " << yaw << " yaw (vec: " << cameraView.x << "x " << cameraView.y << "y " << cameraView.z << "z)").str().c_str());
	ImGui::Combo("Movetype", &global::moveType, " walk\0 fly\0 noclip\0");
	ImGui::Combo("Hull", &global::hullIndex, "regular player (0)\0 ducked player (1)\0 point hull (2)\0 3\0");
	ImGui::End();

	ImGui::Begin("Log");
	for (const auto& line : m_console)
		ImGui::Text(line.c_str());
	ImGui::End();

	ImGui::Begin("Render");
	ImGui::Combo("render API", (int*)&global::renderApi,
#ifdef WIN32
		"OpenGL\0Direct3D\0"
#else
		"OpenGL\0"
#endif
	);

	ImGui::Checkbox("textures", &global::textures);
	ImGui::Checkbox("lightmaps", &global::lightmaps);
	ImGui::Checkbox("polygons", &global::polygons);

	ImGui::Checkbox("staticBSP", &global::renderStaticBSP);
	ImGui::Checkbox("brushEntities", &global::renderBrushEntities);
	ImGui::Checkbox("skybox", &global::renderSkybox);
	ImGui::Checkbox("decals", &global::renderDecals);
	ImGui::Checkbox("coords", &global::renderCoords);
	ImGui::Checkbox("leafOutlines", &global::renderLeafOutlines);
	ImGui::Checkbox("HUD", &global::renderHUD);
	ImGui::End();

	ImGui::Render();

	return ImGui::GetDrawData();
}

auto Hud::fontHeight() const -> int {
	return FONT_HUD_HEIGHT;
}

auto Hud::fontColor() const -> glm::vec3 {
	return FONT_HUD_COLOR;
}
