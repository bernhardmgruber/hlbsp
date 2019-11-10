#include "HudRenderable.h"

#include "Hud.h"
#include "IRenderer.h"
#include "global.h"

HudRenderable::HudRenderable(render::IRenderer& renderer, const Hud& hud)
	: m_renderer(renderer), m_hud(hud) {
}
void HudRenderable::render(const RenderSettings& settings) {
	if (!global::renderHUD)
		return;
	m_renderer.renderImgui(m_hud.drawData());
}
