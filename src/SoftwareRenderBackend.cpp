//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "SoftwareRenderBackend.h"

using namespace dae;

SoftwareRenderBackend::SoftwareRenderBackend(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height] { FLT_MAX };
}

SoftwareRenderBackend::~SoftwareRenderBackend()
{
	delete[] m_pDepthBufferPixels;
}

int dae::SoftwareRenderBackend::GetWidth() const {
    return m_Width;
}

int dae::SoftwareRenderBackend::GetHeight() const {
    return m_Height;
}

void SoftwareRenderBackend::Render(const Camera& camera, std::vector<Mesh*>& meshes)
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	// Reset screen to default
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	std::fill_n(m_pBackBufferPixels, m_Width * m_Height, SDL_MapRGB(m_pBackBuffer->format,
																																	static_cast<uint8_t>(m_BackgroundColor.r * 255),
																																	static_cast<uint8_t>(m_BackgroundColor.b * 255),
																																	static_cast<uint8_t>(m_BackgroundColor.g * 255)));

	//RENDER LOGICs
	for (Mesh* mesh : meshes) {
		if (!mesh->CanBeSoftwareRendered() || !mesh->Visible()) {
			continue;
		}

		VertexTransformationFunction(camera, mesh);
		RenderMesh(camera, mesh);
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void SoftwareRenderBackend::VertexTransformationFunction(const Camera& camera, Mesh* mesh) const
{
	const std::vector<Vertex>& inputVertices = mesh->GetVertices();
	auto& outVertices{ mesh->GetOutVerticesMutable() };

	const Matrix worldViewProjectionMatrix{ mesh->GetWorldMatrix() * camera.invViewMatrix * camera.projectionMatrix };

	for (size_t index{}; index < inputVertices.size(); ++index) {
		OutVertex& outVertex{ outVertices[index] };
		outVertex.position = worldViewProjectionMatrix.TransformPoint(inputVertices[index].position.ToPoint4());

		outVertex.position.x /= outVertex.position.w;
		outVertex.position.y /= outVertex.position.w;
		outVertex.position.z /= outVertex.position.w;

		// Convert from ndc to screenspace position
		outVertex.position.x = ((outVertex.position.x + 1) / 2) * m_Width;
		outVertex.position.y = ((1 - outVertex.position.y) / 2) * m_Height;

		outVertex.color = inputVertices[index].color;
		outVertex.uv = inputVertices[index].uv;
		outVertex.normal = mesh->GetWorldMatrix().TransformVector(inputVertices[index].normal).Normalized();
		outVertex.tangent = mesh->GetWorldMatrix().TransformVector(inputVertices[index].tangent).Normalized();
		outVertex.viewDirection = (camera.origin - mesh->GetWorldMatrix().TransformPoint(inputVertices[index].position)).Normalized();
	}
}

void dae::SoftwareRenderBackend::RenderMesh(const Camera& camera, const Mesh* mesh) {
	auto& vertices = mesh->GetOutVertices();

	switch (mesh->GetTopology()) {
		case Mesh::PrimitiveTopology::TriangleStrip:
		{
			const std::vector<uint32_t>& indices = mesh->GetIndices();

			for (size_t index{}; index < indices.size() - 2; ++index) {
				bool uneven{ static_cast<bool>(index & 1) };

				OutVertex v0{ vertices[indices[index]] };
				OutVertex v1{ vertices[indices[index + 1]] };
				OutVertex v2{ vertices[indices[index + 2]] };

				if (!uneven) {
					std::swap(v1, v2);
				}

				RenderTriangle(camera, mesh, v0, v1, v2);
			}

			break;
		}

		default:
		{
			const std::vector<uint32_t>& indices = mesh->GetIndices();

			for (size_t index{}; index < indices.size(); index += 3) {
				RenderTriangle(camera, mesh, vertices[indices[index]], vertices[indices[index + 1]], vertices[indices[index + 2]]);
			}

			break;
		}
	}
}

void dae::SoftwareRenderBackend::RenderTriangle(const Camera& camera, const Mesh* mesh, const OutVertex& v0, const OutVertex& v1, const OutVertex& v2) {	
	if (v0.position == v1.position || v1.position == v2.position || v2.position == v0.position) {
		return;
	}

	if (!(v0.position.z > 0 && v0.position.z < 1) || !(v1.position.z > 0 && v1.position.z < 1) || !(v2.position.z > 0 && v2.position.z < 1)) {
		return;
	}

	const Vector2 t0{ v0.position.GetXY() };
	const Vector2 t1{ v1.position.GetXY() };
	const Vector2 t2{ v2.position.GetXY() };

	Vector2 edge0{ t1 - t0 };
	Vector2 edge1{ t2 - t1 };
	Vector2 edge2{ t0 - t2 };

	const float normMagnitude{ Vector2::Cross(edge0, t2 - t0) };

	switch (mesh->GetCullMode()) {
		case Mesh::CullMode::BackFace:
		{
			if (normMagnitude < 0.0f) {
				return;
			};

			break;
		}

		case Mesh::CullMode::FrontFace:
		{
			if (normMagnitude > 0.0f) {
				return;
			};

			break;
		}

		default:
			// No culling applied
			break;
	}


	const float invMagnitude{ 1 / normMagnitude };

	// Calculate triangle bounding box in screen space
	float minX = std::min({ v0.position.x, v1.position.x, v2.position.x });
	float minY = std::min({ v0.position.y, v1.position.y, v2.position.y });
	float maxX = std::max({ v0.position.x, v1.position.x, v2.position.x });
	float maxY = std::max({ v0.position.y, v1.position.y, v2.position.y });

	// Clamp bounding box to screen dimensions
	int startX = std::max(static_cast<int>(std::floor(minX)), 0);
	int startY = std::max(static_cast<int>(std::floor(minY)), 0);
	int endX = std::min(static_cast<int>(std::ceil(maxX)), m_Width);
	int endY = std::min(static_cast<int>(std::ceil(maxY)), m_Height);

	for (int py{ startY }; py < endY; ++py) {
		for (int px{ startX }; px < endX; ++px) {
			if (m_ShowBoundingBox) {
				// Fill the bounding box with white
				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
																															static_cast<uint8_t>(255),
																															static_cast<uint8_t>(255),
																															static_cast<uint8_t>(255));
				continue;
			}

			const Vector2 pixelCenter{ px + 0.5f, py + 0.5f };

			Vector2 toPFromV0{ pixelCenter - t0 };
			Vector2 toPFromV1{ pixelCenter - t1 };
			Vector2 toPFromV2{ pixelCenter - t2 };

			// Compute the weights
			const float weight0 = Vector2::Cross(edge1, toPFromV1) * invMagnitude;
			if (weight0 < 0.f) {
				continue;
			}

			const float weight1 = Vector2::Cross(edge2, toPFromV2) * invMagnitude;
			if (weight1 < 0.f) {
				continue;
			}

			const float weight2 = Vector2::Cross(edge0, toPFromV0) * invMagnitude;
			if (weight2 < 0.f) {
				continue;
			}

			const Vector3 weightedPos = v0.position * weight0 + v1.position * weight1 + v2.position * weight2;
			if (weightedPos.z >= m_pDepthBufferPixels[py + m_Height * px] || weightedPos.z < FLT_EPSILON) {
				continue;
			}

			const float interpolatedZ{ 1 / ((weight0 / v0.position.z) + (weight1 / v1.position.z) + (weight2 / v2.position.z)) };
			if (interpolatedZ <= 0 || interpolatedZ > 1) {
				continue;
			}

			const float interpolatedW{ 1 / ((weight0 / v0.position.w) + (weight1 / v1.position.w) + (weight2 / v2.position.w)) };

			Vector2 uv = ((v0.uv / v0.position.w) * weight0 +
										(v1.uv / v1.position.w) * weight1 +
										(v2.uv / v2.position.w) * weight2) * interpolatedW;

			ColorRGB finalColor{};

			switch (m_ViewMode) {
				case ViewMode::depthBuffer:
				{
					const float remapedInterpolatedZ{ Remap(interpolatedZ, 0.985f, 1.f) };
					finalColor = { remapedInterpolatedZ, remapedInterpolatedZ, remapedInterpolatedZ };
					break;
				}

				default:
				{
					const ColorRGB color{ mesh->GetDiffuse()->Sample(uv) };

					const Vector3 normal{ v0.normal * weight0 + v1.normal * weight1 + v2.normal * weight2 };
					const Vector3 tangent{ v0.tangent * weight0 + v1.tangent * weight1 + v2.tangent * weight2 };
					const Vector3 position{ v0.position * weight0 + v1.position * weight1 + v2.position * weight2 };
					const Vector3 viewDirection{ v0.viewDirection * weight0 + v1.viewDirection * weight1 + v2.viewDirection * weight2 };

					finalColor = PixelShading(mesh, { position.ToPoint4(), color, uv, normal.Normalized(), tangent.Normalized(), viewDirection.Normalized() });
					break;
				}
			}

			const ColorRGB ambient{ 0.025f, 0.025f, 0.025f };
			finalColor = finalColor + ambient;
			finalColor.MaxToOne();

			m_pDepthBufferPixels[py + m_Height * px] = interpolatedZ;
			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
																														static_cast<uint8_t>(finalColor.r * 255),
																														static_cast<uint8_t>(finalColor.g * 255),
																														static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

ColorRGB dae::SoftwareRenderBackend::PixelShading(const Mesh* mesh, const OutVertex& vertex) const {
	const Vector3 lightDirection{ .577f, -.577f, .577f };

	const float lightIntensity{ 7.f };
	const float shininess{ 25.f };

	Vector3 normal{ vertex.normal };

	if (m_NormalMapEnabled) {
		const Vector3 binormal{ Vector3::Cross(vertex.normal, vertex.tangent).Normalized() };
		const Matrix tangentSpaceAxis{ vertex.tangent, binormal, vertex.normal, Vector3::Zero };
		ColorRGB sampledNormalColor{ mesh->GetNormal()->Sample(vertex.uv) };

		Vector3 sampledNormal{ sampledNormalColor.r, sampledNormalColor.g, sampledNormalColor.b };
		sampledNormal = 2.f * sampledNormal - Vector3{ 1.f, 1.f, 1.f };

		normal = tangentSpaceAxis.TransformVector(sampledNormal).Normalized();
	}

	switch (m_ShadingMode) {
		case dae::SoftwareRenderBackend::ShadingMode::observedArea: {
			const float observedArea{ std::max(Vector3::Dot(normal, -lightDirection), 0.f) };
			return { colors::White * observedArea };
		}

		case dae::SoftwareRenderBackend::ShadingMode::diffuse: {
			return{ (lightIntensity * vertex.color) / static_cast<float>(M_PI) };
		}

		case dae::SoftwareRenderBackend::ShadingMode::specular: {
			ColorRGB glossinessColor{ mesh->GetGlossiness()->Sample(vertex.uv) };
			glossinessColor.MaxToOne();

			float glossiness{ glossinessColor.r * shininess };
			const ColorRGB specularColor{ mesh->GetSpecular()->Sample(vertex.uv) };

			const Vector3 reflect{ Vector3::Reflect(lightDirection, normal) };

			const float angle{ std::max(Vector3::Dot(reflect, vertex.viewDirection), 0.f) };
			const float specReflection{ powf(angle, glossiness) };

			return{ specReflection * specularColor };
		}

		default: {
			ColorRGB color{ vertex.color };
			color.MaxToOne();

			const ColorRGB lambertDiffuse{ (lightIntensity * color) / static_cast<float>(M_PI) };
			const float observedArea{ std::max(Vector3::Dot(normal, -lightDirection), 0.f) };

			ColorRGB glossinessColor{ mesh->GetGlossiness()->Sample(vertex.uv) };
			glossinessColor.MaxToOne();

			float glossiness{ glossinessColor.r * shininess };
			const ColorRGB specularColor{ mesh->GetSpecular()->Sample(vertex.uv) };

			const Vector3 reflect{ Vector3::Reflect(lightDirection, normal) };
			const float angle{ std::max(Vector3::Dot(reflect, vertex.viewDirection), 0.f) };
			const float specReflection{ powf(angle, glossiness) };

			const ColorRGB phong{ specReflection * specularColor };

			ColorRGB outColor{ (lambertDiffuse * observedArea) + phong };
			outColor.MaxToOne();

			return outColor;
		}
	}
}

float dae::SoftwareRenderBackend::Remap(float value, float newMin, float newMax) const {
	return (newMax - newMin) / (newMax - value);
}

void dae::SoftwareRenderBackend::CycleViewMode() {
	switch (m_ViewMode) {
		case ViewMode::finalColor:
			m_ViewMode = ViewMode::depthBuffer;
			std::cout << "Switched to depth buffer view" << std::endl;
			break;
		case ViewMode::depthBuffer:
			m_ViewMode = ViewMode::finalColor;
			std::cout << "Switched to color view" << std::endl;
	}
}

void dae::SoftwareRenderBackend::CycleShadingMode() {
	switch (m_ShadingMode) {
		case dae::SoftwareRenderBackend::ShadingMode::observedArea:
			m_ShadingMode = ShadingMode::diffuse;
			std::cout << "Switched to software mode diffuse" << std::endl;
			break;
		case dae::SoftwareRenderBackend::ShadingMode::diffuse:
			m_ShadingMode = ShadingMode::specular;
			std::cout << "Switcheded to software mode specular" << std::endl;
			break;
		case dae::SoftwareRenderBackend::ShadingMode::specular:
			m_ShadingMode = ShadingMode::combined;
			std::cout << "Switcheded to software mode combined" << std::endl;
			break;
		case dae::SoftwareRenderBackend::ShadingMode::combined:
			m_ShadingMode = ShadingMode::observedArea;
			std::cout << "Swicheded to software mode observed area" << std::endl;
			break;
	}
}

void dae::SoftwareRenderBackend::ToggleNormalMap() {
	m_NormalMapEnabled = !m_NormalMapEnabled;

	if (m_NormalMapEnabled) {
		std::cout << "Enabled normal map" << std::endl;
	} else {
		std::cout << "Disabled normal map" << std::endl;
	}
}

void dae::SoftwareRenderBackend::ToggleBoundingBox() {
	m_ShowBoundingBox = !m_ShowBoundingBox;

	if (m_ShowBoundingBox) {
		std::cout << "Showing bounding box" << std::endl;
	} else {
		std::cout << "Hiding bounding box" << std::endl;
	}
}
