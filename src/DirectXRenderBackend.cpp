#include "pch.h"
#include "DirectXRenderBackend.h"
#include "Utils.h"
#include "MeshEffect.h"
#include <memory>

namespace dae {

	DirectXRenderBackend::DirectXRenderBackend(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//Initialize DirectX pipeline
		const HRESULT result = InitializeDirectX();
		if (result == S_OK)
		{
			m_IsInitialized = true;
			std::cout << "DirectX is initialized and ready!\n";
		}
		else
		{
			std::cout << "DirectX initialization failed!\n";
		}
	}

	DirectXRenderBackend::~DirectXRenderBackend()
	{
		if (m_pDeviceContext) {
			m_pDeviceContext->ClearState();
			m_pDeviceContext->Flush();
			m_pDeviceContext->Release();
		}
	}

	int DirectXRenderBackend::GetWidth() const {
		return m_Width;
	}

	int DirectXRenderBackend::GetHeight() const {
		return m_Height;
	}

	void DirectXRenderBackend::Render(const Camera& camera, std::vector<Mesh*>& meshes)
	{
		if (!m_IsInitialized)
			return;

		// Clear RTV & DSV
		float color[4]{ m_BackgroundColor.r, m_BackgroundColor.g, m_BackgroundColor.b, 1.f };
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, color);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		// Update mesh position based on camera
		Vector3 cameraOrigin{ camera.origin };

		for (Mesh* mesh : meshes) {
			if (!mesh->Visible()) {
				continue;
			}

			Matrix worldViewProjectionMatrix{ mesh->GetWorldMatrix() * camera.invViewMatrix * camera.projectionMatrix };

			mesh->BindDevice(m_pDevice); // Binds the mesh to the device if it has not already been bound

			auto effect = mesh->GetEffect();
			effect->SetWorldViewProjectionVariable(worldViewProjectionMatrix);
			effect->SetCameraPositionVariable(cameraOrigin);

			mesh->Draw(m_pDeviceContext, m_EffectTechnique);
		}

		m_pSwapChain->Present(0, 0);
	}

	void DirectXRenderBackend::SwitchTechnique() {
		switch (m_EffectTechnique) {
			case BaseEffect::TechniqueType::Point:
				m_EffectTechnique = BaseEffect::TechniqueType::Linear;
				std::cout << "Switched to linear technique" << std::endl;
				break;
			case BaseEffect::TechniqueType::Linear:
				m_EffectTechnique = BaseEffect::TechniqueType::Anisotropic;
				std::cout << "Switched to anisotropic technique" << std::endl;
				break;
			case BaseEffect::TechniqueType::Anisotropic:
				m_EffectTechnique = BaseEffect::TechniqueType::Point;
				std::cout << "Switched to point technique" << std::endl;
				break;
		}
	}

	ID3D11Device* DirectXRenderBackend::GetDevice() {
		return m_pDevice;
	}

	HRESULT DirectXRenderBackend::InitializeDirectX()
	{
		D3D_FEATURE_LEVEL featurelevel{ D3D_FEATURE_LEVEL_11_1 };
		uint32_t createDeviceFlags{};

#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		// Create device and device context
		HRESULT result{ D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featurelevel, 1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext) };

		if (FAILED(result)) {
			return result;
		}

		// Create DXGI factory
		IDXGIFactory1* pDxgiFactory{};
		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pDxgiFactory));
		if (FAILED(result)) {
			return result;
		}

		// Swapchain settings
		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;

		SDL_SysWMinfo sysWMInfo{};
		SDL_GetVersion(&sysWMInfo.version);
		SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);
		swapChainDesc.OutputWindow = sysWMInfo.info.win.window;

		// Create swapchain
		result = pDxgiFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
		if (FAILED(result)) {
			return result;
		}

		// Depth buffer stuff, depth stencil magic tomfoolery
		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		// View
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);
		if (FAILED(result)) {
			return result;
		}

		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
		if (FAILED(result)) {
			return result;
		}

		// Resource
		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));
		if (FAILED(result)) {
			return result;
		}

		// View
		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);
		if (FAILED(result)) {
			return result;
		}

		// Bind RTV & DSV to Output Merger Stage
		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);
	
		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(m_Width);
		viewport.Height = static_cast<float>(m_Height);
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		m_pDeviceContext->RSSetViewports(1, &viewport);

		return result;
	}
}
