#pragma once
#include "pch.h"

#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>

#include <vector>
#include "Math.h"
#include "BaseEffect.h"
#include "MeshEffect.h"
#include "Vector2.h"
#include "Vector4.h"
#include "Texture.h"
#include "Matrix.h"
#include <memory>

using dae::Vector4;
using dae::Vector3;
using dae::ColorRGB;
using dae::Vector2;
using dae::Texture;

struct Vertex {
	Vector3 position;
	ColorRGB color;
	Vector2 uv;
	Vector3 normal;
	Vector3 tangent;
};

struct OutVertex {
	Vector4 position{};
	ColorRGB color{};
	Vector2 uv{};
	Vector3 normal{};
	Vector3 tangent{};
	Vector3 viewDirection{};
};

class Mesh {
public:
	enum class PrimitiveTopology {
		TriangleList,
		TriangleStrip
	};

	// Each mesh can have its own culling mode
	enum class CullMode {
		BackFace,
		FrontFace,
		None
	};

	// Takes ownership of vertices and indices
	Mesh(PrimitiveTopology topology, std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::shared_ptr<BaseEffect> pBaseEffect);
	~Mesh();

	// Meshes should not be copied
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;

	void BindDevice(ID3D11Device* pDevice);
	void Draw(ID3D11DeviceContext* pDeviceContext, BaseEffect::TechniqueType technique) const;

	const std::vector<Vertex>& GetVertices() const;
	const std::vector<uint32_t>& GetIndices() const;

	void SetWorldMatrix(dae::Matrix matrix);

	const dae::Matrix& GetWorldMatrix() const;

	PrimitiveTopology GetTopology() const;

	std::shared_ptr<BaseEffect> GetEffect() const;

	std::shared_ptr<Texture> GetDiffuse() const;
	std::shared_ptr<Texture> GetNormal() const;
	std::shared_ptr<Texture> GetSpecular() const;
	std::shared_ptr<Texture> GetGlossiness() const;

	void ToggleCullMode();
	void SetCullMode(CullMode mode);
	CullMode GetCullMode() const;

	void SetDiffuse(std::shared_ptr<Texture> pTexture);
	void SetNormal(std::shared_ptr<Texture> pTexture);
	void SetSpecular(std::shared_ptr<Texture> pTexture);
	void SetGlossiness(std::shared_ptr<Texture> pTexture);

	bool CanBeSoftwareRendered() const;
	void DisableSoftwareRendering();

	std::vector<OutVertex>& GetOutVerticesMutable();
	const std::vector<OutVertex>& GetOutVertices() const;

	bool Visible() const;
	void SetVisible(bool val);
private:
	std::vector<Vertex> m_Vertices;
	std::vector<OutVertex> m_OutVertices;
	std::vector<uint32_t> m_Indices;
	PrimitiveTopology m_PrimitiveTopology;

	dae::Matrix m_WorldMatrix;

	uint32_t m_NumIndices;

	// The shader for the mesh
	std::shared_ptr<BaseEffect> m_pEffect;

	// Textures are now stored per mesh given the way the render backends function
	std::shared_ptr<Texture> m_pDiffuseTexture{ nullptr };
	std::shared_ptr<Texture> m_pNormalTexture{ nullptr };
	std::shared_ptr<Texture> m_pSpecularTexture{ nullptr };
	std::shared_ptr<Texture> m_pGlossinessTexture{ nullptr };

	ID3D11Buffer* m_pVertexBuffer{ nullptr };
	ID3D11Buffer* m_pIndexBuffer{ nullptr };

	CullMode m_CullMode{};

	bool m_IsBound{ false };
	bool m_CanBeSoftwareRendered{ true };
	bool m_Visible{ true };
};