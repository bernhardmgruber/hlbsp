#include "GlfwWindow.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <atomic>

namespace {
	std::atomic<unsigned int> g_guiCount = 0;

	auto gui(GLFWwindow* w) -> GlfwWindow& {
		return *static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
	}
}

GlfwWindow::GlfwWindow(std::string windowTitle, std::function<GLFWwindow*(int width, int height, const char* title, GLFWmonitor* monitor)> createWindowAndContext)
	: m_windowTitle(std::move(windowTitle)) {
	// set GLFW error callback before any other GLFW function call
	glfwSetErrorCallback(onError);

	// if this is the first window, init GLFW
	{
		if (g_guiCount++ == 0)
			if (!glfwInit())
				throw std::runtime_error("failed to init GLFW");
	}

	m_window = createWindowAndContext(m_width, m_height, windowTitle.c_str(), nullptr);

	onWindowCreated();
}

GlfwWindow::~GlfwWindow() {
	glfwDestroyWindow(m_window);

	// if this is the last window, terminate GLFW
	if (g_guiCount-- == 1)
		glfwTerminate();
}

auto GlfwWindow::handle() const -> GLFWwindow* {
	return m_window;
}

auto GlfwWindow::shouldClose() const -> bool {
	return glfwWindowShouldClose(m_window);
}

void GlfwWindow::toggleFullscreen() {
	// determine new monitor and window resolution
	GLFWmonitor* monitor = nullptr;
	int newWidth, newHeight;
	if (m_fullscreen) {
		// revert to window resolution
		newWidth = m_windowedWidth;
		newHeight = m_windowedHeight;
	} else {
		int x, y;
		glfwGetWindowPos(m_window, &x, &y);

		const GLFWvidmode* mode;
		std::tie(monitor, mode) = [&] {
			int count;
			auto monitors = glfwGetMonitors(&count);
			if (count < 1)
				throw std::runtime_error{"Failed to query monitors"};

			// find monitor with biggest overlap with current window
			auto maxArea = 0;
			const GLFWvidmode* resultMode = nullptr;
			GLFWmonitor* resultMonitor = nullptr;

			for (auto i = 0; i < count; i++) {
				const auto& mon = monitors[i];
				int mx, my;
				glfwGetMonitorPos(mon, &mx, &my);
				const auto mode = glfwGetVideoMode(mon);
				if (!mode)
					throw std::runtime_error{"Failed to query monitor video mode"};
				const auto x1 = std::max(mx, x);
				const auto x2 = std::min(mx + mode->width, x + m_width);
				const auto y1 = std::max(my, y);
				const auto y2 = std::min(my + mode->height, y + m_height);
				const auto area = (x2 - x1) * (y2 - y1);
				if (area > maxArea) {
					maxArea = area;
					resultMode = mode;
					resultMonitor = mon;
				}
			}
			assert(resultMonitor);
			assert(resultMode);
			return std::make_pair(resultMonitor, resultMode);
		}();

		// store original window resolution and position
		m_windowedWidth = m_width;
		m_windowedHeight = m_height;
		m_windowedX = x;
		m_windowedY = y;

		// apply monitor resolution
		newWidth = mode->width;
		newHeight = mode->height;
	}

	// create new window, sharing resources from previous window, and destroy old one
	const auto newWindow = glfwCreateWindow(newWidth, newHeight, m_windowTitle.c_str(), monitor, m_window);
	glfwSetWindowPos(newWindow, m_windowedX, m_windowedY);
	glfwDestroyWindow(m_window);
	m_window = newWindow;

	// we set m_width and m_height after the windows have been created and closed as these actions trigger resize events which may corrupt m_width and m_height
	m_width = newWidth;
	m_height = newHeight;

	onWindowCreated();

	m_fullscreen = !m_fullscreen;
}

void GlfwWindow::toggleStereo() {
	glfwWindowHint(GLFW_STEREO, !m_stereo);

	// if we are fullscreen, create the new window in fullscreen mode on the same monitor again
	auto monitor = glfwGetWindowMonitor(m_window);

	// copy m_width and m_height as creating and closing the windows triggers resize events which change m_width and m_height
	const auto w = m_width;
	const auto h = m_height;

	const auto newWindow = glfwCreateWindow(w, h, m_windowTitle.c_str(), monitor, m_window);
	if (!m_stereo && !newWindow)
		throw std::runtime_error("Failed to switch to stereo mode. Maybe your system does not support stereo rendering?");

	glfwSetWindowPos(newWindow, m_windowedX, m_windowedY);
	glfwDestroyWindow(m_window);

	m_width = w;
	m_height = h;

	m_stereo = !m_stereo;
}

void GlfwWindow::onError(int error, const char* description) {
	std::cerr << "GLFW error " << error << ": " << description << '\n';
}

void GlfwWindow::onResize(GLFWwindow* window, int width, int height) {
	auto& g = gui(window);
	g.m_width = width;
	g.m_height = height;
	g.onResize(width, height);
}

void GlfwWindow::onMouseButton(GLFWwindow* window, int button, int action, int modifiers) {
	gui(window).onMouseButton(button, action, modifiers);
}

void GlfwWindow::onMouseMove(GLFWwindow* window, double dx, double dy) {
	gui(window).onMouseMove(dx, dy);
}

void GlfwWindow::onMouseWheel(GLFWwindow* window, double xOffset, double yOffset) {
	gui(window).onMouseWheel(xOffset, yOffset);
}

void GlfwWindow::onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
	gui(window).onKey(key, scancode, action, mods);
}

void GlfwWindow::onChar(GLFWwindow* window, unsigned int codepoint) {
	gui(window).onChar(codepoint);
}

void GlfwWindow::onWindowCreated() {
	if (m_window == nullptr) {
		throw std::runtime_error("failed to open window");
		return;
	}

	glfwSetWindowUserPointer(m_window, this);

	// after the window has been created, we have an OpenGL context
	glfwMakeContextCurrent(m_window);

	// configure GLFW context
	glfwSwapInterval(0); // vsync

	glfwSetWindowSizeCallback(m_window, onResize);
	glfwSetMouseButtonCallback(m_window, onMouseButton);
	glfwSetCursorPosCallback(m_window, onMouseMove);
	glfwSetScrollCallback(m_window, onMouseWheel);
	glfwSetKeyCallback(m_window, onKey);
	glfwSetCharCallback(m_window, onChar);
}
