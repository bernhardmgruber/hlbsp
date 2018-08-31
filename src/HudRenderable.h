#pragma once

#include "IRenderable.h"
#include "font.h"
#include "opengl/Buffer.h"
#include "opengl/Program.h"
#include "opengl/Texture.h"
#include "opengl/VAO.h"

class Hud;
class Camera;

class HudRenderable : public IRenderable {
public:
	HudRenderable(const Hud& hud, const Camera& camera);

	virtual void render(const RenderSettings& settings) override;

private:
	void renderText(gl::Buffer& buffer, int nX, int nY, const std::string& text, float sx = 1, float sy = 1);

	const Hud& m_hud;
	const Camera& m_camera;

	Font m_font;
	gl::Texture m_fontTex;

	gl::VAO m_fontVao;
	gl::Program m_fontProgram;
	gl::Buffer m_textBuffer;
};