#pragma once

enum class RenderAPI : int {
	OpenGL,
	Direct3D
};

namespace global {
	inline auto renderApi = RenderAPI::OpenGL;
}
