#include "Shader.h"

#include <d3dcompiler.h>

#include "../../IO.h"

namespace dx11 {
	Shader::Shader(const std::string& profile, const std::string& entryPoint, const std::string& source, const std::string& filename) {
		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
		flags |= D3DCOMPILE_DEBUG;
		flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		ComPtr<ID3DBlob> errorBlob;
		if (FAILED(D3DCompile(source.c_str(), source.size(), filename.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), profile.c_str(), flags, 0, &m_blob, &errorBlob))) {
			auto msg = "Failed to compile shader " + filename + ":\n";
			if (errorBlob)
				msg += static_cast<const char*>(errorBlob->GetBufferPointer());
			throw std::runtime_error(msg);
		}
	}

	Shader::Shader(const std::string& profile, const std::string& entryPoint, const fs::path& file)
		: Shader(profile, entryPoint, readTextFile(file), file.string()) {}

	Shader::Shader(Shader&& other) {
		swap(other);
	}

	Shader& Shader::operator=(Shader&& other) {
		swap(other);
		return *this;
	}

	Shader::~Shader() = default;

	auto Shader::blob() -> ID3DBlob* {
		return m_blob.Get();
	}

	auto Shader::blob() const -> const ID3DBlob* {
		return m_blob.Get();
	}

	void Shader::swap(Shader& other) {
		using std::swap;
		swap(m_blob, other.m_blob);
	}

	VertexShader::VertexShader(ID3D11Device* device, const std::string& profile, const std::string& entryPoint, const std::string& source, const std::string& filename)
		: Shader(profile, entryPoint, source, filename) {
		if (FAILED(device->CreateVertexShader(m_blob->GetBufferPointer(), m_blob->GetBufferSize(), nullptr, &m_shader)))
			throw std::runtime_error("Failed to create vertex shader from compiled bytecode");
	}

	VertexShader::VertexShader(ID3D11Device* device, const std::string& profile, const std::string& entryPoint, const fs::path& file)
		: VertexShader(device, profile, entryPoint, readTextFile(file), file.string()) {}

	auto VertexShader::shader() -> ID3D11VertexShader* {
		return m_shader.Get();
	}

	PixelShader::PixelShader(ID3D11Device* device, const std::string& profile, const std::string& entryPoint, const std::string& source, const std::string& filename)
		: Shader(profile, entryPoint, source, filename) {
		if (FAILED(device->CreatePixelShader(m_blob->GetBufferPointer(), m_blob->GetBufferSize(), nullptr, &m_shader)))
			throw std::runtime_error("Failed to create vertex shader from compiled bytecode");
	}

	PixelShader::PixelShader(ID3D11Device* device, const std::string& profile, const std::string& entryPoint, const fs::path& file)
		: PixelShader(device, profile, entryPoint, readTextFile(file), file.string()) {}

	auto PixelShader::shader() -> ID3D11PixelShader* {
		return m_shader.Get();
	}
}
