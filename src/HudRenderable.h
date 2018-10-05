#pragma once

#include "IRenderable.h"

namespace render {
	class IRenderer;
}
class Hud;

class HudRenderable : public IRenderable {
public:
	HudRenderable(render::IRenderer& renderer, const Hud& hud);

	virtual void render(const RenderSettings& settings) override;

private:
	render::IRenderer& m_renderer;
	const Hud& m_hud;
};