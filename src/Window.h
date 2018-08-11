#pragma once

#include <GL/glew.h>
#include <glm/vec2.hpp>

#include "GlfwWindow.h"
#include "camera.h"
#include "Bsp.h"
#include "hud.h"
#include "timer.h"
#include "GLRenderer.h"

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

	Bsp    bsp;
	Hud    hud;
	Timer  timer;
	Camera camera;

	RenderSettings m_settings;
	GLRenderer m_renderer;

	bool       m_captureMouse = false;
	glm::dvec2 m_mouseDownPos;
};
