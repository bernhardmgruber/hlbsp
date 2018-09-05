#include "Hud.h"

#include "Camera.h"
#include "IPSS.h"
#include "Timer.h"

namespace {
	constexpr auto FONT_HUD_HEIGHT = 12;
	constexpr auto FONT_HUD_SPACE = 5;
	constexpr glm::vec3 FONT_HUD_COLOR = {1.0f, 0.0f, 0.0f};

	constexpr auto CONSOLE_WIDTH = 400;
	constexpr auto CONSOLE_HEIGHT = 300;
}

Hud::Hud(const Camera& camera, const Timer& timer)
	: m_camera(camera), m_timer(timer) {}

void Hud::print(std::string text) {
	m_console.push_back(std::move(text));
}

auto Hud::texts() const -> std::vector<Text> {
	std::vector<Text> result;

	const auto& cameraPos = m_camera.position;
	const auto& cameraView = m_camera.viewVector();
	const auto& pitch = m_camera.pitch;
	const auto& yaw = m_camera.yaw;
	const auto& fps = m_timer.TPS;

	int nCurrentY = m_camera.viewportHeight;

	result.push_back({FONT_HUD_SPACE, nCurrentY -= (FONT_HUD_SPACE + FONT_HUD_HEIGHT),
		IPSS() << std::fixed << std::setprecision(1) << "FPS: " << fps});
	result.push_back({FONT_HUD_SPACE, nCurrentY -= (FONT_HUD_SPACE + FONT_HUD_HEIGHT),
		IPSS() << std::fixed << std::setprecision(1) << "Cam pos: " << cameraPos.x << "x " << cameraPos.y << "y " << cameraPos.z << "z"});
	result.push_back({FONT_HUD_SPACE, nCurrentY -= (FONT_HUD_SPACE + FONT_HUD_HEIGHT),
		IPSS() << std::fixed << std::setprecision(1) << "Cam view: " << pitch << "°pitch " << yaw << "°yaw (vec: " << cameraView.x << "x " << cameraView.y << "y " << cameraView.z << "z)"});

	// console
	nCurrentY = FONT_HUD_SPACE;
	for (const auto& line : m_console) {
		if (nCurrentY + FONT_HUD_HEIGHT >= CONSOLE_HEIGHT)
			break;
		result.push_back({FONT_HUD_SPACE, nCurrentY, line});
		nCurrentY += FONT_HUD_HEIGHT + FONT_HUD_SPACE;
	}

	return result;
}

auto Hud::fontHeight() const -> int {
	return FONT_HUD_HEIGHT;
}

auto Hud::fontColor() const -> glm::vec3 {
	return FONT_HUD_COLOR;
}
