#pragma once

#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>

#include "Mesh.h"
#include "Camera.h"
#include "BaseEffect.h"
#include "FireMeshEffect.h"
#include "AbstractRenderBackend.h"
#include "ColorRGB.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class DirectXRenderBackend final : public AbstractRenderBackend
	{
	public:
		DirectXRenderBackend(SDL_Window* pWindow);
		~DirectXRenderBackend();

		int GetWidth() const override;
		int GetHeight() const override;

		void Render(const Camera& camera, std::vector<Mesh*>& meshes) override;

		void SwitchTechnique();

		ID3D11Device* GetDevice();
	private:
		HRESULT InitializeDirectX();

		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };

		BaseEffect::TechniqueType m_EffectTechnique{};

		//DIRECTX
		ID3D11Device* m_pDevice{ nullptr };
		ID3D11DeviceContext* m_pDeviceContext{ nullptr };
		ID3D11Texture2D* m_pDepthStencilBuffer{ nullptr };
		ID3D11DepthStencilView* m_pDepthStencilView{ nullptr };
		ID3D11Resource* m_pRenderTargetBuffer{ nullptr };
		ID3D11RenderTargetView* m_pRenderTargetView{ nullptr };
		IDXGISwapChain* m_pSwapChain{ nullptr };
	};
}
