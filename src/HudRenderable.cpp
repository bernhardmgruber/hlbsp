#include "HudRenderable.h"

#include <iomanip>

#include "mathlib.h"
#include "IPSS.h"
#include "Hud.h"
#include "Camera.h"
#include "Timer.h"

namespace {
	constexpr auto FONT_HUD_HEIGHT = 12;
	constexpr auto FONT_HUD_SPACE = 5;
	constexpr float FONT_HUD_COLOR[] = { 1.0f, 0.0f, 0.0f };

	constexpr auto CONSOLE_WIDTH = 400;
	constexpr auto CONSOLE_HEIGHT = 300;
}

HudRenderable::HudRenderable(const Hud& hud, const Camera& camera, const Timer& timer)
	: m_hud(hud)
	, m_camera(camera)
	, m_timer(timer) {

	m_font = Font("../../data/fonts/helvetica.ttf", FONT_HUD_HEIGHT);

	m_fontProgram = gl::Program{
		gl::Shader(GL_VERTEX_SHADER, std::experimental::filesystem::path{"../../src/shader/font.vert"}),
		gl::Shader(GL_FRAGMENT_SHADER, std::experimental::filesystem::path{"../../src/shader/font.frag"})
	};
}

void HudRenderable::render(const RenderSettings& settings) {
	if (!settings.renderHUD)
		return;

	const auto matrix = glm::ortho<float>(0, m_camera.viewportWidth, 0, m_camera.viewportHeight, -1.0f, 1.0f);

	m_fontVao.bind();
	m_fontProgram.use();
	glUniformMatrix4fv(m_fontProgram.uniformLocation("projection"), 1, false, glm::value_ptr(matrix));
	glUniform1i(m_fontProgram.uniformLocation("tex"), 0);
	glUniform3fv(m_fontProgram.uniformLocation("textColor"), 1, FONT_HUD_COLOR);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	const auto& cameraPos = m_camera.position;
	const auto& cameraView = m_camera.viewVector();
	const auto& pitch = m_camera.pitch;
	const auto& yaw = m_camera.yaw;
	const auto& fps = m_timer.TPS;

	int nCurrentY = m_camera.viewportHeight;

	glPuts(FONT_HUD_SPACE, nCurrentY -= (FONT_HUD_SPACE + FONT_HUD_HEIGHT), m_font,
		IPSS() << std::fixed << std::setprecision(1) << "FPS: " << fps);
	glPuts(FONT_HUD_SPACE, nCurrentY -= (FONT_HUD_SPACE + FONT_HUD_HEIGHT), m_font,
		IPSS() << std::fixed << std::setprecision(1) << "Cam pos: " << cameraPos.x << "x " << cameraPos.y << "y " << cameraPos.z << "z");
	glPuts(FONT_HUD_SPACE, nCurrentY -= (FONT_HUD_SPACE + FONT_HUD_HEIGHT), m_font,
		IPSS() << std::fixed << std::setprecision(1) << "Cam view: " << pitch << "°pitch " << yaw << "°yaw (vec: " << cameraView.x << "x " << cameraView.y << "y " << cameraView.z << "z)");

	// console
	nCurrentY = FONT_HUD_SPACE;
	for (const auto& line : m_hud.console()) {
		if (nCurrentY + FONT_HUD_HEIGHT >= CONSOLE_HEIGHT)
			break;
		glPuts(FONT_HUD_SPACE, nCurrentY, m_font, line);
		nCurrentY += FONT_HUD_HEIGHT + FONT_HUD_SPACE;
	}

	glDisable(GL_BLEND);
}
