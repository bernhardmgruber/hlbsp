#pragma once

#include "Bsp.h"
#include "GLRenderer.h"
#include "GlfwWindow.h"
#include "camera.h"
#include "hud.h"
#include "timer.h"

class Window : public GlfwWindow {
public:
	Window();
	~Window();

	void update();
	void draw();

private:
	virtual void onResize(int width, int height) override;
	virtual void onMouseButton(int button, int action, int modifiers) override;
	virtual void onMouseMove(double dx, double dy) override;
	virtual void onMouseWheel(double xOffset, double yOffset) override;
	virtual void onKey(int key, int scancode, int action, int mods) override;
	virtual void onChar(unsigned int codepoint) override;

	Timer timer;
	Camera camera;
	Hud hud;
	Bsp bsp;

	RenderSettings m_settings;
	GLRenderer m_renderer;

	bool m_captureMouse = false;
	glm::dvec2 m_mouseDownPos;
};
