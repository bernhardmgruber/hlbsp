#include "HudRenderable.h"

#include "Hud.h"
#include "IRenderer.h"

HudRenderable::HudRenderable(render::IRenderer& renderer, const Hud& hud)
	: m_renderer(renderer), m_hud(hud) {
}
void HudRenderable::render(const RenderSettings& settings) {
	if (!settings.renderHUD)
		return;
	m_renderer.renderImgui(m_hud.drawData());
}
