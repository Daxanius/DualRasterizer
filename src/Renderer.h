#pragma once

#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>

#include "Mesh.h"
#include "Camera.h"
#include "AbstractRenderBackend.h"
#include "Texture.h"
#include "memory"

namespace dae {
	// A base renderer which manages the common data (meshes) for render backends
	class Renderer final {
	public:
		Renderer(AbstractRenderBackend* pRenderBackend);

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void ToggleRotation();

		void Update(const Timer* pTimer);
		void Render();

		void SetRenderBackend(AbstractRenderBackend* pRenderBackend);

		std::vector<Mesh*>& GetMeshes();
	protected:
		AbstractRenderBackend* m_pRenderBackend;

		Camera m_Camera{};

		bool m_RotationEnabled{ false };

		float m_Rotation{ 0.f };

		std::vector<Mesh*> m_WorldMeshes;
	};
}