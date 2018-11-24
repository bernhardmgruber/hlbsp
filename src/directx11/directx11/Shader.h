#pragma once

#ifdef _WIN32
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

#include <filesystem>

namespace fs = std::filesystem;

namespace dx11 {
	class Shader {
	public:
		Shader() = default;
		Shader(const std::string& profile, const std::string& entryPoint, const std::string& source, const std::string& filename = "");
		Shader(const std::string& profile, const std::string& entryPoint, const fs::path& file);
		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;
		Shader(Shader&& other);
		Shader& operator=(Shader&& other);
		~Shader();

		auto blob() -> ID3DBlob*;
		auto blob() const -> const ID3DBlob*;

	protected:
		void swap(Shader& other);

		ComPtr<ID3DBlob> m_blob;
	};

	class VertexShader : public Shader {
	public:
		VertexShader() = default;
		VertexShader(ID3D11Device* device, const std::string& profile, const std::string& entryPoint, const std::string& source, const std::string& filename = "");
		VertexShader(ID3D11Device* device, const std::string& profile, const std::string& entryPoint, const fs::path& file);

		auto shader() -> ID3D11VertexShader*;

	private:
		ComPtr<ID3D11VertexShader> m_shader;
	};

	class PixelShader : public Shader {
	public:
		PixelShader() = default;
		PixelShader(ID3D11Device* device, const std::string& profile, const std::string& entryPoint, const std::string& source, const std::string& filename = "");
		PixelShader(ID3D11Device* device, const std::string& profile, const std::string& entryPoint, const fs::path& file);

		auto shader() -> ID3D11PixelShader*;

	private:
		ComPtr<ID3D11PixelShader> m_shader;
	};
}