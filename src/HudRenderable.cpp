#include "HudRenderable.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>

#include "Camera.h"
#include "Hud.h"

HudRenderable::HudRenderable(const Hud& hud, const Camera& camera)
	: m_hud(hud), m_camera(camera) {
	ImGui_ImplOpenGL3_Init("#version 330");
	ImGui_ImplOpenGL3_NewFrame(); // trigger building of some resources
}

HudRenderable::~HudRenderable() {
	ImGui_ImplOpenGL3_Shutdown();
}

void HudRenderable::render(const RenderSettings& settings) {
	if (!settings.renderHUD)
		return;
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplOpenGL3_RenderDrawData(m_hud.drawData());
}
