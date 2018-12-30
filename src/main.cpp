#include <iostream>

#ifdef _WIN32
#include "directx11/Renderer.h"
#endif
#include "opengl/Renderer.h"
#include "Bsp.h"
#include "Window.h"

auto main(const int argc, const char* argv[]) -> int try {
	if (argc != 2)
		throw std::runtime_error("Missing map name as command line argument");

	const auto mapFile = argv[1];
	Bsp bsp(mapFile);

	auto platform = std::make_unique<render::opengl::Platform>();
	//auto platform = std::make_unique<render::directx11::Platform>();
	Window window(*platform, bsp);

	while (true) {
		glfwPollEvents();
		if (glfwGetKey(window.handle(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
			break;
		if (window.shouldClose())
			break;

		window.update();
		window.draw();
		platform->swapBuffers();
	}

	return 0;
} catch (const std::exception& e) {
	std::cerr << "Exception: " << e.what() << "\n";
	return 1;
} catch (...) {
	std::cerr << "Unknown exception\n";
	return 1;
}