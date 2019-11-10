#include "Renderer.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>

#include <iostream>

#include "../IRenderable.h"
#include "../Camera.h"
#include "../Entity.h"
#include "../mathlib.h"
#include "../global.h"
#include "../Bsp.h"

#include "directx11/Shader.h"

namespace render::directx11 {
	struct alignas(16) ConstantBufferData {
		glm::mat4 m;
		alignas(4) int unit1Enabled;
		alignas(4) int unit2Enabled;
		//alignas(4) int flashlight;
		alignas(4) int nightvision;
		alignas(4) int alphaTest;
	};

	namespace {
		auto channelsToTextureType(const Image& img) {
			switch (img.channels) {
				case 1: return DXGI_FORMAT_R8_UNORM;
				case 2: return DXGI_FORMAT_R8G8_UNORM;
				case 4: return DXGI_FORMAT_R8G8B8A8_UNORM;
				default: assert(false);
			}
		}

		auto convert(AttributeLayout::Type type, unsigned int size) {
			switch (type) {
			case AttributeLayout::Type::Float:
				switch (size) {
				case 1: return DXGI_FORMAT_R32_FLOAT;
				case 2: return DXGI_FORMAT_R32G32_FLOAT;
				case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
				case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
				}
			}
			assert(false);
		}

		template <typename T>
		auto createConstantBuffer(ID3D11Device* device) {
			D3D11_BUFFER_DESC desc{};
			desc.ByteWidth = sizeof(T);
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

			if (desc.ByteWidth % 16 != 0)
				throw std::logic_error("constant buffer size must be a multiple of 16 bytes");
			if (desc.ByteWidth >= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT)
				throw std::logic_error("constant buffer is too large");

			ComPtr<ID3D11Buffer> cb;
			if (FAILED(device->CreateBuffer(&desc, nullptr, &cb)))
				throw std::runtime_error("Failed to create buffer");

			return cb;
		}
	}

	struct Texture : ITexture {
		ComPtr<ID3D11Texture2D> t;
		ComPtr<ID3D11ShaderResourceView> srv;
	};

	struct Buffer : IBuffer {
		ComPtr<ID3D11Buffer> b;
	};

	struct InputLayout : IInputLayout  {
		Buffer* b;
		ComPtr<ID3D11InputLayout> l;
		UINT stride;
	};

	Renderer::Renderer(Platform& platform)
		: m_device(platform.m_device)
		, m_context(platform.m_context)
		, m_swapChain(platform.m_swapChain)
		, m_backBufferRTV(platform.m_backBufferRTV)
		//, m_fbColorTexture(platform.m_fbColorTexture)
		, m_fbDepthStencilTexture(platform.m_fbDepthStencilTexture)
		//, m_fbRtv(platform.m_fbRtv)
		//, m_fbSrv(platform.m_fbSrv)
		, m_fbDsv(platform.m_fbDsv) {

		D3D11_RASTERIZER_DESC rasterizerDesc{};
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_FRONT;
		rasterizerDesc.FrontCounterClockwise = true;
		rasterizerDesc.DepthClipEnable = true;
		rasterizerDesc.MultisampleEnable = true;
		if (FAILED(m_device->CreateRasterizerState(&rasterizerDesc, &m_defaultRasterizerState)))
			throw std::runtime_error("Failed to create default rasterizer state");
		m_context->RSSetState(m_defaultRasterizerState.Get());

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		if (FAILED(m_device->CreateDepthStencilState(&depthStencilDesc, &m_defaultDepthStencilState)))
			throw std::runtime_error("Failed to create default depth stencil state");

		depthStencilDesc.DepthEnable = false;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		if (FAILED(m_device->CreateDepthStencilState(&depthStencilDesc, &m_skyboxDepthStencilState)))
			throw std::runtime_error("Failed to create skybox depth stencil state");

		D3D11_SAMPLER_DESC samplerDes{};
		samplerDes.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDes.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDes.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDes.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDes.MaxAnisotropy = 1;
		samplerDes.MinLOD = 0;
		samplerDes.MaxLOD = D3D11_FLOAT32_MAX;
		if (FAILED(m_device->CreateSamplerState(&samplerDes, &m_wrapSampler)))
			throw std::runtime_error("Failed to create default sampler state");

		samplerDes.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDes.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDes.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		if (FAILED(m_device->CreateSamplerState(&samplerDes, &m_skyboxSampler)))
			throw std::runtime_error("Failed to create skybox sampler state");

		m_matrixCBuffer = createConstantBuffer<glm::mat4>(m_device.Get());
		m_mainCBuffer = createConstantBuffer<ConstantBufferData>(m_device.Get());
		
		m_skyboxProgram = dx11::Program{
			dx11::VertexShader(m_device.Get(), "vs_5_0", "main_vs", fs::path{"../src/directx11/shader/skybox.hlsl"}),
			dx11::PixelShader(m_device.Get(), "ps_5_0", "main_ps", fs::path{"../src/directx11/shader/skybox.hlsl"}),
		};

		m_shaderProgram = dx11::Program{
			dx11::VertexShader(m_device.Get(), "vs_5_0", "main_vs", fs::path{"../src/directx11/shader/main.hlsl"}),
			dx11::PixelShader(m_device.Get(), "ps_5_0", "main_ps", fs::path{"../src/directx11/shader/main.hlsl"}),
		};

		m_coordsProgram = dx11::Program{
			dx11::VertexShader(m_device.Get(), "vs_5_0", "main_vs", fs::path{"../src/directx11/shader/coords.hlsl"}),
			dx11::PixelShader(m_device.Get(), "ps_5_0", "main_ps", fs::path{"../src/directx11/shader/coords.hlsl"}),
		};

		ImGui_ImplDX11_Init(m_device.Get(), m_context.Get());
		ImGui_ImplDX11_NewFrame(); // trigger building of some resources
	}

	Renderer::~Renderer() {
		ImGui_ImplDX11_Shutdown();
	}

	void Renderer::resizeViewport(int width, int height) {
		m_context->OMSetRenderTargets(0, nullptr, nullptr);

		// Release all outstanding references to the swap chain's buffers.
		m_backBufferRTV->Release();

		// Preserve the existing buffer count and format.
		// Automatically choose the width and height to match the client rect for HWNDs.
		if (FAILED(m_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0)))
			throw std::runtime_error("");

		{
			ComPtr<ID3D11Texture2D> backBuffer;
			if (FAILED(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
				throw std::runtime_error("");

			if (FAILED(m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_backBufferRTV.GetAddressOf())))
				throw std::runtime_error("");
		}

		m_context->OMSetRenderTargets(1, m_backBufferRTV.GetAddressOf(), m_fbDsv.Get());

		D3D11_VIEWPORT viewport{};
		viewport.Width = width;
		viewport.Height = height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		m_context->RSSetViewports(1, &viewport);
	}

	void Renderer::clear() {
		constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_context->ClearRenderTargetView(m_backBufferRTV.Get(), clearColor);
		m_context->ClearDepthStencilView(m_fbDsv.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	auto Renderer::createTexture(const std::vector<Image>& mipmaps) const -> std::unique_ptr<ITexture> {
		if (mipmaps.front().channels == 3) {
			// create 4 channel images
			std::vector<Image> mipmapsWithAlpha;
			mipmapsWithAlpha.resize(mipmaps.size());
			std::transform(begin(mipmaps), end(mipmaps), begin(mipmapsWithAlpha), [] (const Image& img){
				return Image(img, 4);
			});
			return createTexture(mipmapsWithAlpha);
		}

		std::unique_ptr<Texture> t(new Texture());

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = mipmaps.front().width;
		desc.Height = mipmaps.front().height;
		desc.MipLevels = mipmaps.size();
		desc.ArraySize = 1;
		desc.Format = channelsToTextureType(mipmaps.front());
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		std::vector<D3D11_SUBRESOURCE_DATA> subresources;
		subresources.reserve(mipmaps.size());
		for (const auto& mm : mipmaps) {
			auto& sr = subresources.emplace_back();
			sr.pSysMem = mm.data.data();
			sr.SysMemPitch = mm.channels * mm.width;
			sr.SysMemSlicePitch = 0;
		}

		if (FAILED(m_device->CreateTexture2D(&desc, subresources.data(), &t->t)))
			throw std::runtime_error("Failed to create 2D texture");

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		if (FAILED(m_device->CreateShaderResourceView(t->t.Get(), &srvDesc, &t->srv)))
			throw std::runtime_error("Failed to create 2D texture shader resource view");

		return t;
	}

	auto Renderer::createCubeTexture(const std::array<Image, 6>& sides) const -> std::unique_ptr<ITexture> {
		if (sides.front().channels == 3) {
			// create 4 channel images
			std::array<Image, 6> sidesAlpha;
			std::transform(begin(sides), end(sides), begin(sidesAlpha), [](const Image& img) {
				return Image(img, 4);
			});
			return createCubeTexture(sidesAlpha);
		}

		std::unique_ptr<Texture> t(new Texture());

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = sides[0].width;
		desc.Height = sides[0].height;
		desc.MipLevels = 1;
		desc.ArraySize = 6;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.CPUAccessFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		std::vector<D3D11_SUBRESOURCE_DATA> subresources;
		subresources.reserve(sides.size());
		for (const auto& mm : sides) {
			auto& sr = subresources.emplace_back();
			sr.pSysMem = mm.data.data();
			sr.SysMemPitch = mm.channels * mm.width;
			sr.SysMemSlicePitch = 0;
		}

		if (FAILED(m_device->CreateTexture2D(&desc, subresources.data(), &t->t)))
			throw std::runtime_error("Failed to create cube map texture");

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;
		if (FAILED(m_device->CreateShaderResourceView(t->t.Get(), &srvDesc, &t->srv)))
			throw std::runtime_error("Failed to create cube map texture shader resource view");

		return t;
	}

	auto Renderer::createBuffer(std::size_t size, const void* data) const -> std::unique_ptr<IBuffer> {
		std::unique_ptr<Buffer> b(new Buffer());

		D3D11_BUFFER_DESC desc{};
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.ByteWidth = size;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA sr;
		sr.pSysMem = data;
		sr.SysMemPitch = 0;
		sr.SysMemSlicePitch = 0;

		if (FAILED(m_device->CreateBuffer(&desc, &sr, &b->b)))
			throw std::runtime_error("Failed to create buffer");

		return b;
	}

	auto Renderer::createInputLayout(IBuffer& buffer, const std::vector<AttributeLayout>& layout) const -> std::unique_ptr<IInputLayout> {
		std::unique_ptr<InputLayout> l(new InputLayout);
		l->b = static_cast<Buffer*>(&buffer);
		l->stride = layout.empty() ? 0 : layout.front().stride;

		std::vector<D3D11_INPUT_ELEMENT_DESC> desc;
		desc.reserve(layout.size());
		for (const auto& al : layout)
			desc.push_back({ al.semantic, al.semanticIndex, convert(al.type, al.size), 0, al.offset, D3D11_INPUT_PER_VERTEX_DATA, 0 });

		auto* vsBytecode = const_cast<ID3DBlob*>(m_shaderProgram.vertexShader().blob());
		if (FAILED(m_device->CreateInputLayout(desc.data(), desc.size(), vsBytecode->GetBufferPointer(), vsBytecode->GetBufferSize(), &l->l)))
			throw std::runtime_error("Failed to create input layout");

		return l;
	}

	void Renderer::renderCoords(const glm::mat4& matrix) {
		m_context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
		m_context->IASetInputLayout(nullptr);
		m_context->VSSetShader(m_coordsProgram.vertexShader().shader(), nullptr, 0);
		m_context->PSSetShader(m_coordsProgram.pixelShader().shader(), nullptr, 0);
		m_context->OMSetDepthStencilState(m_defaultDepthStencilState.Get(), 0);

		m_context->UpdateSubresource(m_matrixCBuffer.Get(), 0, nullptr, &matrix, 0, 0);
		m_context->VSSetConstantBuffers(0, 1, m_matrixCBuffer.GetAddressOf());

		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		m_context->Draw(12, 0);
	}

	void Renderer::renderSkyBox(ITexture& cubemap, const glm::mat4& matrix) {
		m_context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
		m_context->IASetInputLayout(nullptr);
		m_context->VSSetShader(m_skyboxProgram.vertexShader().shader(), nullptr, 0);
		m_context->PSSetShader(m_skyboxProgram.pixelShader().shader(), nullptr, 0);
		m_context->PSSetShaderResources(0, 1, static_cast<Texture&>(cubemap).srv.GetAddressOf());
		m_context->OMSetDepthStencilState(m_skyboxDepthStencilState.Get(), 0);
		m_context->PSSetSamplers(0, 1, m_skyboxSampler.GetAddressOf());

		m_context->UpdateSubresource(m_matrixCBuffer.Get(), 0, nullptr, &matrix, 0, 0);
		m_context->VSSetConstantBuffers(0, 1, m_matrixCBuffer.GetAddressOf());

		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_context->Draw(36, 0);
	}

	void Renderer::renderStatic(std::vector<EntityData> entities, const std::vector<Decal>& decals, IInputLayout& staticLayout, IInputLayout& decalLayout, std::vector<std::unique_ptr<render::ITexture>>& textures, render::ITexture& lightmapAtlas, const RenderSettings& settings) {
		const UINT offset = 0;
		m_context->IASetInputLayout(static_cast<InputLayout&>(staticLayout).l.Get());
		m_context->IASetVertexBuffers(0, 1, static_cast<InputLayout&>(staticLayout).b->b.GetAddressOf(), &static_cast<InputLayout&>(staticLayout).stride, &offset);
		m_context->VSSetShader(m_shaderProgram.vertexShader().shader(), nullptr, 0);
		m_context->PSSetShader(m_shaderProgram.pixelShader().shader(), nullptr, 0);
		m_context->OMSetDepthStencilState(m_defaultDepthStencilState.Get(), 0);
		m_context->PSSetSamplers(0, 1, m_wrapSampler.GetAddressOf());

		ConstantBufferData cbd{
			settings.projection * settings.view,
			global::textures,
			global::lightmaps,
			//global::flashlight,
			global::nightvision,
			false
		};

		for (auto& ent : entities)
			renderBrushEntity(std::move(ent.fri), lightmapAtlas, settings, ent.origin, ent.alpha, ent.renderMode, cbd);

		//if (settings.renderDecals) {
		//	cbd.unit2Enabled = false;
		//	auto cb = createConstantBuffer(m_device.Get(), cbd);
		//	m_context->VSSetConstantBuffers(0, 1, cb.GetAddressOf());
		//	m_context->IASetInputLayout(static_cast<InputLayout&>(decalLayout).l.Get());
		//	m_context->IASetVertexBuffers(0, 1, static_cast<InputLayout&>(decalLayout).b->b.GetAddressOf(), &static_cast<InputLayout&>(decalLayout).stride, &offset);

		//	renderDecals(decals, textures);
		//}
	}

	void Renderer::renderBrushEntity(std::vector<FaceRenderInfo> fri, render::ITexture& lightmapAtlas, const RenderSettings& settings, glm::vec3 origin, float alpha, bsp30::RenderMode renderMode, ConstantBufferData cbd) {
		cbd.m = glm::translate(settings.projection * settings.view, origin);
		cbd.alphaTest = renderMode == bsp30::RENDER_MODE_SOLID;

		//switch (renderMode) {
		//case bsp30::RENDER_MODE_TEXTURE:
		//	glEnable(GL_BLEND);
		//	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		//	glDepthMask(GL_FALSE);
		//	break;
		//case bsp30::RENDER_MODE_ADDITIVE:
		//	glEnable(GL_BLEND);
		//	glBlendFunc(GL_ONE, GL_ONE);
		//	glDepthMask(GL_FALSE);
		//	break;
		//}

		m_context->UpdateSubresource(m_mainCBuffer.Get(), 0, nullptr, &cbd, 0, 0);
		m_context->VSSetConstantBuffers(0, 1, m_mainCBuffer.GetAddressOf());
		m_context->PSSetConstantBuffers(0, 1, m_mainCBuffer.GetAddressOf());

		renderFri(std::move(fri), lightmapAtlas);

		//switch (renderMode) {
		//case bsp30::RENDER_MODE_TEXTURE:
		//case bsp30::RENDER_MODE_ADDITIVE:
		//	glDisable(GL_BLEND);
		//	glDepthMask(GL_TRUE);
		//	break;
		//}
	}

	void Renderer::renderFri(std::vector<FaceRenderInfo> fri, render::ITexture& lightmapAtlas) {
		// sort by texture id to avoid some rebinds
		//std::sort(begin(fri), end(fri), [](const FaceRenderInfo& a, const FaceRenderInfo& b) {
		//	return a.tex < b.tex;
		//});

		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_context->PSSetShaderResources(1, 1, static_cast<Texture&>(lightmapAtlas).srv.GetAddressOf());
		ITexture* curId = nullptr;
		for (const auto& i : fri) {
			if (curId != i.tex) {
				m_context->PSSetShaderResources(0, 1, static_cast<Texture&>(*i.tex).srv.GetAddressOf());
				curId = i.tex;
			}
			m_context->Draw(i.count, i.offset);
		}
	}

	void Renderer::renderDecals(const std::vector<Decal>& decals, std::vector<std::unique_ptr<render::ITexture>>& textures) {
		//glEnable(GL_POLYGON_OFFSET_FILL);
		//glPolygonOffset(0.0f, -2.0f);

		D3D11_RENDER_TARGET_BLEND_DESC rtbd{};
		rtbd.BlendEnable = true;

		rtbd.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		rtbd.BlendOp = D3D11_BLEND_OP_ADD;

		rtbd.SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		rtbd.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		D3D11_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = true;
		blendDesc.RenderTarget[0] = rtbd;

		ComPtr<ID3D11BlendState> blendState;
		if (FAILED(m_device->CreateBlendState(&blendDesc, &blendState)))
			throw std::runtime_error("failed to create blend state");

		m_context->OMSetBlendState(blendState.Get(), nullptr, 0);

		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // TODO FAN

		for (auto i = 0; i < decals.size(); i++) {
			m_context->PSSetShaderResources(0, 1, static_cast<Texture&>(*textures[decals[i].texIndex]).srv.GetAddressOf());
			m_context->Draw(4, i * 4);
		}

		m_context->OMSetBlendState(nullptr, nullptr, 0);

		//glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void Renderer::renderImgui(ImDrawData* data) {
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplDX11_RenderDrawData(data);
	}

	auto Renderer::screenshot() const -> Image {
		return Image();
	}

	auto Platform::createWindowAndContext(int width, int height, const char * title, GLFWmonitor * monitor) -> GLFWwindow* {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		auto window = glfwCreateWindow(width, height, title, monitor, nullptr);
		auto hwnd = glfwGetWin32Window(window);

		UINT deviceFlags = 0;
#if _DEBUG
		deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		const auto featureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
		if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags, &featureLevel, 1, D3D11_SDK_VERSION, &m_device, nullptr, &m_context)))
			throw std::runtime_error("Failed to create D3D11 device");

		ComPtr<IDXGIDevice> dxgiDevice;
		if (FAILED(m_device.As<IDXGIDevice>(&dxgiDevice)))
			throw std::runtime_error("Device is not a IDXGIDevice");

		ComPtr<IDXGIAdapter> dxgiAdapter;
		if (FAILED(dxgiDevice->GetAdapter(&dxgiAdapter)))
			throw std::runtime_error("Failed to get IDXGIAdapter");

		ComPtr<IDXGIFactory1> dxgiFactory;
		DXGI_ADAPTER_DESC adapterDesc;
		if (FAILED(dxgiAdapter->GetDesc(&adapterDesc)))
			throw std::runtime_error("Failed to get adapter description");
		if (FAILED(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))))
			throw std::runtime_error("Failed to get adapter parent");
		if (!dxgiFactory)
			throw std::runtime_error("Failed to retrieve the IDXGIFactory1 interface associated with D3D11 device");

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferCount = 1;
		swapChainDesc.BufferDesc.Width = width;
		swapChainDesc.BufferDesc.Height = height;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.OutputWindow = hwnd;
		swapChainDesc.Windowed = true;

		if (FAILED(dxgiFactory->CreateSwapChain(m_device.Get(), &swapChainDesc, &m_swapChain)))
			throw std::runtime_error("Failed to create the swap chain");
		if (FAILED(dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER)))
			throw std::runtime_error("Failed to make window association");

		{
			ComPtr<ID3D11Texture2D> backBuffer;
			m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
			if (FAILED(m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_backBufferRTV)))
				throw std::runtime_error("Failed to create window back buffer render target view");
		}

		constexpr auto samples = 1;

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = samples;
		desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET;
		if (samples <= 1)
			desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

		//if (FAILED(m_device->CreateTexture2D(&desc, nullptr, &m_fbColorTexture)))
		//	throw std::runtime_error("Failed to create frame buffer color texture");

		//D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
		//rtvDesc.Format = desc.Format;
		//rtvDesc.ViewDimension = (samples > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
		//if (FAILED(m_device->CreateRenderTargetView(m_fbColorTexture.Get(), &rtvDesc, &m_fbRtv)))
		//	throw std::runtime_error("Failed to create frame buffer render target view");

		//if (samples <= 1) {
		//	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		//	srvDesc.Format = desc.Format;
		//	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		//	srvDesc.Texture2D.MostDetailedMip = 0;
		//	srvDesc.Texture2D.MipLevels = 1;
		//	if (FAILED(m_device->CreateShaderResourceView(m_fbColorTexture.Get(), &srvDesc, &m_fbSrv)))
		//		throw std::runtime_error("Failed to create frame buffer shader resource view");
		//}

		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		if (FAILED(m_device->CreateTexture2D(&desc, nullptr, &m_fbDepthStencilTexture)))
			throw std::runtime_error("Failed to create frame buffer depth-stencil texture");

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = desc.Format;
		dsvDesc.ViewDimension = (samples > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
		if (FAILED(m_device->CreateDepthStencilView(m_fbDepthStencilTexture.Get(), &dsvDesc, &m_fbDsv)))
			throw std::runtime_error("Failed to create frame buffer depth-stencil view");

		m_context->OMSetRenderTargets(1, m_backBufferRTV.GetAddressOf(), m_fbDsv.Get());

		D3D11_VIEWPORT viewport{};
		viewport.Width = width;
		viewport.Height = height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		m_context->RSSetViewports(1, &viewport);

		std::wcout << "Direct3D 11 renderer: " << adapterDesc.Description << "\n";

		return window;
	}

	auto Platform::createRenderer() -> std::unique_ptr<IRenderer> {
		return std::make_unique<Renderer>(*this);
	}

	void Platform::swapBuffers() {
		if (FAILED(m_swapChain->Present(1, 0)))
			throw std::runtime_error("Failed to present");
	}
}
