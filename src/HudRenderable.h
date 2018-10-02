#pragma once

#include "IRenderable.h"

class Hud;
class Camera;

class HudRenderable : public IRenderable {
public:
	HudRenderable(const Hud& hud, const Camera& camera);
	~HudRenderable();

	virtual void render(const RenderSettings& settings) override;

private:
	const Hud& m_hud;
	const Camera& m_camera;
};