#pragma once

#include <initializer_list>
#include <string>
#include <unordered_map>

#include "Shader.h"

namespace dx11 {
	class Program {
	public:
		Program() = default;
		Program(VertexShader vs, PixelShader ps);
		Program(const Program&) = delete;
		auto operator=(const Program&) -> Program& = delete;
		Program(Program&& other);
		auto operator=(Program&& other) -> Program&;
		~Program();

		auto vertexShader() -> VertexShader&;
		auto vertexShader() const -> const VertexShader&;
		auto pixelShader() -> PixelShader&;
		auto pixelShader() const -> const PixelShader&;

	private:
		void swap(Program& other);

		VertexShader m_vertexShader;
		PixelShader m_pixelShader;
	};
}