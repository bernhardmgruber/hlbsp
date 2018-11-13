#include "Program.h"

#include <iostream>

#include "../../IO.h"

namespace dx11 {
	Program::Program(VertexShader vs, PixelShader ps)
		: m_vertexShader(std::move(vs)), m_pixelShader(std::move(ps)) {}

	Program::Program(Program&& other)
		: m_vertexShader(std::move(other.m_vertexShader)), m_pixelShader(std::move(other.m_pixelShader)) {}

	Program& Program::operator=(Program&& other) {
		swap(other);
		return *this;
	}

	Program::~Program() = default;

	auto Program::vertexShader() -> VertexShader& {
		return m_vertexShader;
	}

	auto Program::vertexShader() const -> const VertexShader& {
		return m_vertexShader;
	}

	auto Program::pixelShader() -> PixelShader& {
		return m_pixelShader;
	}

	auto Program::pixelShader() const -> const PixelShader& {
		return m_pixelShader;
	}

	void Program::swap(Program& other) {
		using std::swap;
		swap(m_vertexShader, other.m_vertexShader);
		swap(m_pixelShader, other.m_pixelShader);
	}
}