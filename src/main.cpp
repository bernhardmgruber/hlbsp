#include <iostream>

#ifdef _WIN32
#include "directx11/Renderer.h"
#endif
#include "Bsp.h"
#include "Window.h"
#include "global.h"
#include "opengl/Renderer.h"

bool runWithPlatformAPI(const RenderAPI api, Bsp& bsp) {
	auto platform = [&] {
		switch (api) {
			case RenderAPI::OpenGL: return std::unique_ptr<render::IPlatform>{new render::opengl::Platform};
			case RenderAPI::Direct3D: return std::unique_ptr<render::IPlatform>{new render::directx11::Platform};
		}
		std::abort();
	}();

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

		if (global::renderApi != api)
			return true;
	}

	return false;
}

auto main(const int argc, const char* argv[]) -> int try {
	if (argc != 2)
		throw std::runtime_error("Missing map name as command line argument");

	Bsp bsp(argv[1]);

	while (runWithPlatformAPI(global::renderApi, bsp))
		;

	return 0;
} catch (const std::exception& e) {
	std::cerr << "Exception: " << e.what() << "\n";
	return 1;
} catch (...) {
	std::cerr << "Unknown exception\n";
	return 1;
}