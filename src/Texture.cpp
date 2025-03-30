#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>
#include <iostream>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface, ID3D11Device* pDevice) :
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }, m_Width( pSurface->w ), m_Height( pSurface->h ), m_Format(pSurface->format), m_pSurface(pSurface)
	{
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = pSurface->w;
		desc.Height = pSurface->h;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData{};
		initData.pSysMem = m_pSurfacePixels;
		initData.SysMemPitch = static_cast<UINT>(pSurface->pitch);
		initData.SysMemSlicePitch = static_cast<UINT>(pSurface->h * pSurface->pitch);

		HRESULT hr = pDevice->CreateTexture2D(&desc, &initData, &m_pResource);

		if (FAILED(hr)) {
			std::cout << "Failed to create texture \n";
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		SRVDesc.Format = format;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		hr = pDevice->CreateShaderResourceView(m_pResource, &SRVDesc, &m_pResourceView);

		if (FAILED(hr)) {
			std::cout << "Failed to create shader resource view for texture\n";
		}
	}

	Texture::~Texture()
	{
		m_pResourceView->Release();
		m_pResource->Release();
		SDL_FreeSurface(m_pSurface);
	}

	std::shared_ptr<Texture> Texture::LoadFromFile(const std::string& path, ID3D11Device* pDevice)
	{
		return std::make_shared<Texture>(IMG_Load(path.c_str()), pDevice);
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		// Wrap UV coordinates if they exceed [0, 1]
		float u = uv.x - std::floor(uv.x);
		float v = uv.y - std::floor(uv.y);

		const size_t px{ static_cast<size_t>(u * m_Width) };
		const size_t py{ static_cast<size_t>(v * m_Height) };
		const uint32_t pixel{ m_pSurfacePixels[px + (py * m_Width)] };

		Uint8 r{}, g{}, b{};
		SDL_GetRGB(pixel, m_Format, &r, &g, &b);

		return ColorRGB{ r / 255.f, g / 255.f, b / 255.f };
	}

	ID3D11ShaderResourceView* Texture::GetSRV() const {
		return m_pResourceView;
	}
}