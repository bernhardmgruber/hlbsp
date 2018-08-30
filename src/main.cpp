#include <iostream>

#include "Bsp.h"
#include "Window.h"

int main(const int argc, const char* argv[]) try {
	if (argc != 2)
		throw std::runtime_error("Missing map name as command line argument");

	const auto mapFile = argv[1];
	Bsp bsp(mapFile);

	Window window(bsp);

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