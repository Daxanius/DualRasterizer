#include "Mesh.h"

Mesh::Mesh(PrimitiveTopology topology, std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::shared_ptr<BaseEffect> pBaseEffect)
	: m_Vertices(std::move(vertices)), m_Indices(std::move(indices)), m_pIndexBuffer{}, m_NumIndices{ static_cast<uint32_t>(m_Indices.size()) }, m_PrimitiveTopology(topology) {
	
	m_pEffect = pBaseEffect;

	// Preallocate memory for output vertices
	m_OutVertices.assign(m_Vertices.size(), {});
}

Mesh::~Mesh() {
	if (m_pVertexBuffer) {
		m_pVertexBuffer->Release();
	}

	if (m_pIndexBuffer) {
		m_pIndexBuffer->Release();
	}
}

void Mesh::BindDevice(ID3D11Device* pDevice) {
	// Skip binding if it's already bound
	if (m_IsBound) {
		return;
	}

	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(Vertex) * static_cast<uint32_t>(m_Vertices.size());
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = m_Vertices.data();
	HRESULT result{ pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer) };

	if (FAILED(result)) {
		return;
	}

	m_NumIndices = static_cast<uint32_t>(m_Indices.size());
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(uint32_t) * m_NumIndices;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	initData.pSysMem = m_Indices.data();
	result = pDevice->CreateBuffer(&bd, &initData, &m_pIndexBuffer);

	if (FAILED(result)) {
		return;
	}

	m_IsBound = true;
}

void Mesh::Draw(ID3D11DeviceContext* pDeviceContext, BaseEffect::TechniqueType technique) const {
	// Primitive topoligies that are supported
	switch (m_PrimitiveTopology) {
		case PrimitiveTopology::TriangleStrip:
			pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			break;
		default:
			pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			break;
	}

	pDeviceContext->IASetInputLayout(m_pEffect->GetInputLayout());

	constexpr UINT stride{ sizeof(Vertex) };
	constexpr UINT offset = 0;

	pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	D3DX11_TECHNIQUE_DESC techDesc{};

	m_pEffect->GetTechnique(technique)->GetDesc(&techDesc);
	for (UINT p{}; p < techDesc.Passes; ++p) {
		m_pEffect->GetTechnique(technique)->GetPassByIndex(p)->Apply(0, pDeviceContext);
		pDeviceContext->DrawIndexed(m_NumIndices, 0, 0);
	}
}

const std::vector<Vertex>& Mesh::GetVertices() const {
	return m_Vertices;
}

const std::vector<uint32_t>& Mesh::GetIndices() const {
	return m_Indices;
}

void Mesh::SetWorldMatrix(dae::Matrix matrix) {
	m_WorldMatrix = matrix;
	m_pEffect->SetWorldMatrixVariable(m_WorldMatrix);
}

const dae::Matrix& Mesh::GetWorldMatrix() const {
	return m_WorldMatrix;
}

Mesh::PrimitiveTopology Mesh::GetTopology() const {
	return m_PrimitiveTopology;;
}

std::shared_ptr<BaseEffect> Mesh::GetEffect() const {
	return m_pEffect;
}

std::shared_ptr<Texture> Mesh::GetDiffuse() const {
	return m_pDiffuseTexture;
}

std::shared_ptr<Texture> Mesh::GetNormal() const {
	return m_pNormalTexture;
}

std::shared_ptr<Texture> Mesh::GetSpecular() const {
	return m_pSpecularTexture;
}

std::shared_ptr<Texture> Mesh::GetGlossiness() const {
	return m_pGlossinessTexture;
}

void Mesh::ToggleCullMode() {
	switch (m_CullMode) {
		case CullMode::BackFace:
			SetCullMode(m_CullMode = CullMode::FrontFace);
			std::cout << "Switched to frontface culling for mesh" << std::endl;
			break;
		case CullMode::FrontFace:
			SetCullMode(CullMode::None);
			std::cout << "Switched to no culling for mesh" << std::endl;
			break;
		case CullMode::None:
			 SetCullMode(CullMode::BackFace);
			std::cout << "Switched to backface culling for mesh" << std::endl;
			break;
	}
}

void Mesh::SetCullMode(CullMode mode) {
	m_CullMode = mode;

	switch (m_CullMode) {
		case CullMode::BackFace:
			m_pEffect->SetCullMode(D3D11_CULL_BACK);
			break;
		case CullMode::FrontFace:
			m_pEffect->SetCullMode(D3D11_CULL_FRONT);
			break;
		case CullMode::None:
			m_pEffect->SetCullMode(D3D11_CULL_NONE);
			break;
	}
}

Mesh::CullMode Mesh::GetCullMode() const {
	return m_CullMode;
}

void Mesh::SetDiffuse(std::shared_ptr<Texture> pTexture) {
	m_pDiffuseTexture = pTexture;
	m_pEffect->SetDiffuseMap(pTexture);
}

void Mesh::SetNormal(std::shared_ptr<Texture> pTexture) {
	m_pNormalTexture = pTexture;
	m_pEffect->SetNormalMap(pTexture);
}

void Mesh::SetSpecular(std::shared_ptr<Texture> pTexture) {
	m_pSpecularTexture = pTexture;
	m_pEffect->SetSpecularMap(pTexture);
}

void Mesh::SetGlossiness(std::shared_ptr<Texture> pTexture) {
	m_pGlossinessTexture = pTexture;
	m_pEffect->SetGlossinessMap(pTexture);
}

bool Mesh::CanBeSoftwareRendered() const {
	return m_CanBeSoftwareRendered;
}

void Mesh::DisableSoftwareRendering() {
	m_CanBeSoftwareRendered = false;
}

std::vector<OutVertex>& Mesh::GetOutVerticesMutable() {
	return m_OutVertices;
}

const std::vector<OutVertex>& Mesh::GetOutVertices() const {
	return m_OutVertices;
}

bool Mesh::Visible() const {
	return m_Visible;
}

void Mesh::SetVisible(bool val) {
	m_Visible = val;
}
