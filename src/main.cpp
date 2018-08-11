#include <iostream>

#include "Window.h"

int main() try {
	Window window;

	while (true) {
		glfwPollEvents();
		if (glfwGetKey(window.handle(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
			break;
		if (window.shouldClose())
			break;

		window.update();
		window.draw();
		glfwSwapBuffers(window.handle());
	}

	return 0;
} catch (const std::exception& e) {
	std::cerr << "Exception: " << e.what() << "\n";
	return 1;
} catch (...) {
	std::cerr << "Unknown exception\n";
	return 1;
}