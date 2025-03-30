#pragma once
#include <SDL_surface.h>
#include <string>
#include "ColorRGB.h"

#include <d3d11.h>
#include <memory>

namespace dae
{
	struct Vector2;

	class Texture
	{
	public:
		Texture(SDL_Surface* pSurface, ID3D11Device* pDevice);
		~Texture();

		static std::shared_ptr<Texture> LoadFromFile(const std::string& path, ID3D11Device* pDevice);
		ColorRGB Sample(const Vector2& uv) const;

		ID3D11ShaderResourceView* GetSRV() const;
	private:

		ID3D11Texture2D* m_pResource{ nullptr };
		ID3D11ShaderResourceView* m_pResourceView{ nullptr };
		uint32_t* m_pSurfacePixels{ nullptr };
		SDL_PixelFormat* m_Format{ nullptr };
		SDL_Surface* m_pSurface{ nullptr };

		int m_Width;
		int m_Height;
	};
}