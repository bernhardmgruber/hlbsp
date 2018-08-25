#include "HudRenderable.h"

#include <iomanip>

#include "mathlib.h"
#include "Camera.h"
#include "Hud.h"

HudRenderable::HudRenderable(const Hud& hud, const Camera& camera)
	: m_hud(hud), m_camera(camera) {

	m_font = Font("../../data/fonts/helvetica.ttf", m_hud.fontHeight());

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
	glUniform3fv(m_fontProgram.uniformLocation("textColor"), 1, glm::value_ptr(m_hud.fontColor()));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (const auto& text : m_hud.texts())
		renderText(m_textBuffer, text.x, text.y, m_font, text.text);

	glDisable(GL_BLEND);
}
