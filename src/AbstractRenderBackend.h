#pragma once

#include "ColorRGB.h"

namespace dae {
	class AbstractRenderBackend {
	public:
		virtual ~AbstractRenderBackend() = default;

		// Render is not const to allow for optimizations and changes
		virtual void Render(const Camera& camera, std::vector<Mesh*>& meshes) = 0;

		virtual int GetWidth() const = 0;
		virtual int GetHeight() const = 0;

		// Just a color setting function for the renderers
		void SetBackgroundColor(const ColorRGB& color) {
			m_BackgroundColor = color;
		};

		AbstractRenderBackend(const AbstractRenderBackend&) = delete;
		AbstractRenderBackend(AbstractRenderBackend&&) noexcept = delete;
		AbstractRenderBackend& operator=(const AbstractRenderBackend&) = delete;
		AbstractRenderBackend& operator=(AbstractRenderBackend&&) noexcept = delete;
	protected:
		AbstractRenderBackend() = default;

		ColorRGB m_BackgroundColor{};
	};
}