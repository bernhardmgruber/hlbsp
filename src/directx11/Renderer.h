#pragma once

#ifdef _WIN32
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

#include <vector>

#include "../IRenderer.h"
#include "directx11/Program.h"

namespace render::directx11 {
	struct ConstantBufferData;

	class Platform;

	class Renderer : public IRenderer {
	public:
		Renderer(Platform& platform);
		~Renderer();

		virtual void resizeViewport(int width, int height) override;
		virtual void clear() override;

		virtual auto createTexture(const std::vector<Image>& mipmaps) const -> std::unique_ptr<ITexture> override;
		virtual auto createCubeTexture(const std::array<Image, 6>& sides) const -> std::unique_ptr<ITexture> override;
		virtual auto createBuffer(std::size_t size, const void* data) const -> std::unique_ptr<IBuffer> override;
		virtual auto createInputLayout(IBuffer& buffer, const std::vector<AttributeLayout>& layout) const -> std::unique_ptr<IInputLayout> override;

		virtual void renderCoords(const glm::mat4& matrix) override;
		virtual void renderSkyBox(ITexture& cubemap, const glm::mat4& matrix) override;
		virtual void renderStatic(std::vector<EntityData> entities, const std::vector<Decal>& decals, IInputLayout& staticLayout, IInputLayout& decalLayout, std::vector<std::unique_ptr<render::ITexture>>& textures, render::ITexture& lightmapAtlas, const RenderSettings& settings) override;
		virtual void renderImgui(ImDrawData* data) override;

		virtual auto screenshot() const -> Image override;

	private:
		void renderBrushEntity(std::vector<FaceRenderInfo> fri, render::ITexture& lightmapAtlas, const RenderSettings& settings, glm::vec3 origin, float alpha, bsp30::RenderMode renderMode, ConstantBufferData cbd);
		void renderFri(std::vector<FaceRenderInfo> fri, render::ITexture& lightmapAtlas);
		void renderDecals(const std::vector<Decal>& decals, std::vector<std::unique_ptr<render::ITexture>>& textures);

		ComPtr<ID3D11Device>& m_device;
		ComPtr<ID3D11DeviceContext>& m_context;
		ComPtr<IDXGISwapChain>& m_swapChain;
		ComPtr<ID3D11RenderTargetView>& m_backBufferRTV;

		//ComPtr<ID3D11Texture2D>& m_fbColorTexture;
		ComPtr<ID3D11Texture2D>& m_fbDepthStencilTexture;
		//ComPtr<ID3D11RenderTargetView>& m_fbRtv;
		//ComPtr<ID3D11ShaderResourceView>& m_fbSrv;
		ComPtr<ID3D11DepthStencilView>& m_fbDsv;

		ComPtr<ID3D11RasterizerState> m_defaultRasterizerState;
		ComPtr<ID3D11DepthStencilState> m_defaultDepthStencilState;
		ComPtr<ID3D11DepthStencilState> m_skyboxDepthStencilState;

		ComPtr<ID3D11SamplerState> m_wrapSampler;
		ComPtr<ID3D11SamplerState> m_skyboxSampler;

		ComPtr<ID3D11Buffer> m_matrixCBuffer;
		ComPtr<ID3D11Buffer> m_mainCBuffer;

		dx11::Program m_skyboxProgram;
		dx11::Program m_shaderProgram;
		dx11::Program m_coordsProgram;
	};

	class Platform : public IPlatform {
	public:
		virtual auto createWindowAndContext(int width, int height, const char * title, GLFWmonitor * monitor) -> GLFWwindow* override;
		virtual auto createRenderer() -> std::unique_ptr<IRenderer> override;
		virtual void swapBuffers() override;

	private:
		ComPtr<ID3D11Device> m_device;
		ComPtr<ID3D11DeviceContext> m_context;
		ComPtr<IDXGISwapChain> m_swapChain;
		ComPtr<ID3D11RenderTargetView> m_backBufferRTV;

		//ComPtr<ID3D11Texture2D> m_fbColorTexture;
		ComPtr<ID3D11Texture2D> m_fbDepthStencilTexture;
		//ComPtr<ID3D11RenderTargetView> m_fbRtv;
		//ComPtr<ID3D11ShaderResourceView> m_fbSrv;
		ComPtr<ID3D11DepthStencilView> m_fbDsv;

		friend class Renderer;
	};
}