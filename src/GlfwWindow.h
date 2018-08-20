#pragma once

#include <GLFW/glfw3.h>

#include <string>

class GlfwWindow {
public:
	GlfwWindow(std::string windowTitle);
	~GlfwWindow();

	auto handle() const -> GLFWwindow*;
	auto shouldClose() const -> bool;

protected:
	void toggleFullscreen();
	void toggleStereo();

	virtual void onResize(int width, int height) = 0;
	virtual void onMouseButton(int button, int action, int modifiers) = 0;
	virtual void onMouseMove(double dx, double dy) = 0;
	virtual void onMouseWheel(double xOffset, double yOffset) = 0;
	virtual void onKey(int key, int scancode, int action, int mods) = 0;
	virtual void onChar(unsigned int codepoint) = 0;

	bool m_fullscreen = false;
	bool m_stereo = false;
	int m_width = 1024;
	int m_height = 768;

private:
	static void onError(int error, const char* description);
	static void onResize(GLFWwindow*, int width, int height);
	static void onMouseButton(GLFWwindow* window, int button, int action, int modifiers);
	static void onMouseMove(GLFWwindow* window, double dx, double dy);
	static void onMouseWheel(GLFWwindow* window, double ds, double yOffset);
	static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void onChar(GLFWwindow* window, unsigned int codepoint);

	void onWindowCreated();

	// stored when going fullscreen to allow reverting back to same window
	int m_windowedX;
	int m_windowedY;
	int m_windowedWidth;
	int m_windowedHeight;

	GLFWwindow* m_window;
	std::string m_windowTitle;
};