#pragma once

#include <thread>
#include <cstdint>
#include <vector>

#include "Camera.h"
#include "Utils.h"
#include "Texture.h"
#include "AbstractRenderBackend.h"
#include "Mesh.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class SoftwareRenderBackend final : public AbstractRenderBackend
	{
	public:
		enum class ViewMode {
			finalColor,
			depthBuffer
		};

		enum class ShadingMode {
			observedArea,
			diffuse,
			specular,
			combined
		};

		SoftwareRenderBackend(SDL_Window* pWindow);
		~SoftwareRenderBackend();

		int GetWidth() const override;
		int GetHeight() const override;

		void CycleViewMode();
		void CycleShadingMode();
		void ToggleNormalMap();
		void ToggleBoundingBox();

		void Render(const Camera& camera, std::vector<Mesh*>& meshes) override;
	private:
		void VertexTransformationFunction(const Camera& camera, Mesh* mesh) const;
		void RenderMesh(const Camera& camera, const Mesh* mesh);
		void RenderTriangle(const Camera& camera, const Mesh* mesh, const OutVertex& v0, const OutVertex& v1, const OutVertex& v2);
		ColorRGB PixelShading(const Mesh* mesh, const OutVertex& vertex) const;

		float Remap(float value, float newMin, float newMax) const;

		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		ViewMode m_ViewMode{ ViewMode::finalColor };
		ShadingMode m_ShadingMode{ ShadingMode::combined };
		bool m_NormalMapEnabled{ true };
		bool m_ShowBoundingBox{ false };

		float* m_pDepthBufferPixels{};
	};
}
