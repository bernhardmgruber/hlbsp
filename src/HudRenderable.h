#pragma once

#include "IRenderable.h"
#include "font.h"
#include "opengl/Buffer.h"
#include "opengl/Program.h"
#include "opengl/VAO.h"

class Hud;
class Camera;

class HudRenderable : public IRenderable {
public:
	HudRenderable(const Hud& hud, const Camera& camera);

	virtual void render(const RenderSettings& settings) override;

private:
	const Hud& m_hud;
	const Camera& m_camera;

	Font m_font;

	gl::VAO m_fontVao;
	gl::Program m_fontProgram;
	gl::Buffer m_textBuffer;
};