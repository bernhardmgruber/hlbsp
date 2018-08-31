#include "HudRenderable.h"

#include <iomanip>

#include "Camera.h"
#include "Hud.h"
#include "mathlib.h"

HudRenderable::HudRenderable(const Hud& hud, const Camera& camera)
	: m_hud(hud), m_camera(camera) {
	m_font = Font("../../data/fonts/helvetica.ttf", m_hud.fontHeight());

	glBindTexture(GL_TEXTURE_2D, m_fontTex.id());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_font.atlas().width, m_font.atlas().height, 0, GL_RED, GL_UNSIGNED_BYTE, m_font.atlas().data.data());

	m_fontProgram = gl::Program{
		gl::Shader(GL_VERTEX_SHADER, std::experimental::filesystem::path{"../../src/shader/font.vert"}),
		gl::Shader(GL_FRAGMENT_SHADER, std::experimental::filesystem::path{"../../src/shader/font.frag"})};
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
		renderText(m_textBuffer, text.x, text.y, text.text);

	glDisable(GL_BLEND);
}


void HudRenderable::renderText(gl::Buffer& buffer, int x, int y, const std::string& text, float sx, float sy) {
	struct Vertex {
		float x;
		float y;
		float s;
		float t;
	};
	std::vector<Vertex> vertices;
	vertices.reserve(6 * text.size());

	for (const auto& c : text) {
		const auto& g = m_font.glyphs()[std::clamp(static_cast<unsigned char>(c) - 32, 0, 255 - 32)];
		float x2 = x + g.bearing.x * sx;
		float y2 = y + g.bearing.y * sy;
		float w = g.size.x * sx;
		float h = g.size.y * sy;

		// advance the cursor to the start of the next character
		x += g.advance.x * sx;
		y += g.advance.y * sy;

		// skip m_glyphs that have no pixels
		if (!w || !h)
			continue;

		vertices.push_back({ x2, y2, g.texX, 0 });
		vertices.push_back({ x2 + w, y2, g.texX + g.size.x / (float)m_font.atlas().width, 0 });
		vertices.push_back({ x2, y2 - h, g.texX, g.size.y / (float)m_font.atlas().height });
		vertices.push_back({ x2, y2 - h, g.texX, g.size.y / (float)m_font.atlas().height });
		vertices.push_back({ x2 + w, y2, g.texX + g.size.x / (float)m_font.atlas().width, 0 });
		vertices.push_back({ x2 + w, y2 - h, g.texX + g.size.x / (float)m_font.atlas().width, g.size.y / (float)m_font.atlas().height });
	}

	glBindBuffer(GL_ARRAY_BUFFER, buffer.id());
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STREAM_DRAW);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vertex), nullptr);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(2 * sizeof(float)));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_fontTex.id());
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}