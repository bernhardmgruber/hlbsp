#include "Hud.h"

#include <imgui.h>

#include "Camera.h"
#include "IPSS.h"
#include "Timer.h"

namespace {
	constexpr auto FONT_HUD_HEIGHT = 12;
	constexpr auto FONT_HUD_SPACE = 5;
	const glm::vec3 FONT_HUD_COLOR = {1.0f, 0.0f, 0.0f};

	constexpr auto CONSOLE_WIDTH = 400;
	constexpr auto CONSOLE_HEIGHT = 300;
}

Hud::Hud(const Camera& camera, const Timer& timer)
	: m_camera(camera), m_timer(timer) {}

void Hud::print(std::string text) {
	m_console.push_back(std::move(text));
}

auto Hud::drawData() const -> ImDrawData* {
	const auto& cameraPos = m_camera.position;
	const auto& cameraView = m_camera.viewVector();
	const auto& pitch = m_camera.pitch;
	const auto& yaw = m_camera.yaw;
	const auto& fps = m_timer.TPS;

	ImGui::NewFrame();

	ImGui::LabelText("FPS", (IPSS() << std::fixed << std::setprecision(1) << fps).str().c_str());
	ImGui::LabelText("cam pos", (IPSS() << std::fixed << std::setprecision(1) << cameraPos.x << "x " << cameraPos.y << "y " << cameraPos.z << "z").str().c_str());
	ImGui::LabelText("cam view", (IPSS() << std::fixed << std::setprecision(1) << pitch << " pitch " << yaw << " yaw (vec: " << cameraView.x << "x " << cameraView.y << "y " << cameraView.z << "z)").str().c_str());
	ImGui::Begin("Log");
	for (const auto& line : m_console)
		ImGui::Text(line.c_str());
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
