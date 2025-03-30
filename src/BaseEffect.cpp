#include "BaseEffect.h"

BaseEffect::BaseEffect(ID3D11Device* pDevice, const std::wstring& assetFile) : m_pDevice(pDevice) {
	m_pEffect = LoadEffect(pDevice, assetFile);

	m_pPointTechnique = m_pEffect->GetTechniqueByName("PointTechnique");
	if (!m_pPointTechnique->IsValid()) {
		std::wcout << L"Point technique is not valid\n";
		return;
	}

	m_pLinearTechnique = m_pEffect->GetTechniqueByName("LinearTechnique");
	if (!m_pLinearTechnique->IsValid()) {
		std::wcout << L"Linear technique is not valid\n";
		return;
	}

	m_pAnisotropicTechnique = m_pEffect->GetTechniqueByName("AnisotropicTechnique");
	if (!m_pAnisotropicTechnique->IsValid()) {
		std::wcout << L"Anisotropic technique is not valid\n";
		return;
	}

	m_pMatWorldViewProjVariable = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
	if (!m_pMatWorldViewProjVariable->IsValid()) {
		std::wcout << L"m_pMatWorldViewProjVariable is not valid!\n";
	}

	m_pRasterizerVariable = m_pEffect->GetVariableByName("gRasterizerState")->AsRasterizer();
	if (!m_pRasterizerVariable->IsValid()) {
		std::wcout << L"m_pRasterizerVariable is not valid!\n";
	}

  m_pRasterizerVariable->GetRasterizerState(0, &m_pRasterizerState);
}

BaseEffect::~BaseEffect() {
	if (m_pInputLayout) {
		m_pInputLayout->Release();
	}

	if (m_pDiffuseMapVariable) {
		m_pDiffuseMapVariable->Release();
	}

	if (m_pNormalMapVariable) {
		m_pNormalMapVariable->Release();
	}

	if (m_pSpecularMapVariable) {
		m_pSpecularMapVariable->Release();
	}

	if (m_pGlossinessMapVariable) {
		m_pGlossinessMapVariable->Release();
	}

	if (m_pCameraPositionVariable) {
		m_pCameraPositionVariable->Release();
	}

	if (m_pWorldMatrixVariable) {
		m_pWorldMatrixVariable->Release();
	}

	if (m_pPointTechnique) {
		m_pPointTechnique->Release();
	}

	if (m_pLinearTechnique) {
		m_pLinearTechnique->Release();
	}

	if (m_pAnisotropicTechnique) {
		m_pAnisotropicTechnique->Release();
	}

	if (m_pMatWorldViewProjVariable) {
		m_pMatWorldViewProjVariable->Release();
	}

	if (m_pRasterizerVariable) {
		m_pRasterizerVariable->Release();
	}

	if (m_pRasterizerState) {
		m_pRasterizerState->Release();
	}

	if (m_pEffect) {
		m_pEffect->Release();
	}
}

ID3D11InputLayout* BaseEffect::GetInputLayout() const {
	return m_pInputLayout;
}

void BaseEffect::SetWorldMatrixVariable(dae::Matrix& matrix) {
	if (m_pWorldMatrixVariable) {
		m_pWorldMatrixVariable->SetMatrix(reinterpret_cast<float*>(&matrix));
	}
}

void BaseEffect::SetCameraPositionVariable(dae::Vector3& position) {
	if (m_pCameraPositionVariable) {
		m_pCameraPositionVariable->SetFloatVector(reinterpret_cast<float*>(&position));
	}
}

ID3DX11Effect* BaseEffect::GetEffect() const {
	return m_pEffect;
}

ID3DX11EffectTechnique* BaseEffect::GetTechnique(TechniqueType type) const {
	switch (type) {
		case TechniqueType::Point:
			return m_pPointTechnique;

		case TechniqueType::Linear:
			return m_pLinearTechnique;

		case TechniqueType::Anisotropic:
			return m_pAnisotropicTechnique;

		default:
			return m_pPointTechnique;
	}
}

ID3DX11Effect* BaseEffect::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile) {
	HRESULT result;
	ID3D10Blob* pErrorBlob{ nullptr };
	ID3DX11Effect* pEffect;

	DWORD shaderFlags{ 0 };

#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	result = D3DX11CompileEffectFromFile(assetFile.c_str(), nullptr, nullptr, shaderFlags, 0, pDevice, &pEffect, &pErrorBlob);

	if (FAILED(result)) {
		if (pErrorBlob != nullptr) {
			const char* pErrors{ static_cast<char*>(pErrorBlob->GetBufferPointer()) };

			std::wstringstream ss;
			for (unsigned int i{}; i < pErrorBlob->GetBufferSize(); i++) {
				ss << pErrors[i];
			}

			OutputDebugStringW(ss.str().c_str());
			pErrorBlob->Release();
			pErrorBlob = nullptr;

			std::wcout << ss.str() << std::endl;
		} else {
			std::wstringstream ss;
			ss << "EffectLoader: Failed to CreateEffectFromFile!\nPath: " << assetFile;
			std::wcout << ss.str() << std::endl;
			return nullptr;
		}
	}

	return pEffect;
}

void BaseEffect::SetWorldViewProjectionVariable(dae::Matrix& matrix) {
	if (m_pMatWorldViewProjVariable) {
		m_pMatWorldViewProjVariable->SetMatrix(reinterpret_cast<float*>(&matrix));
	}
}

void BaseEffect::SetDiffuseMap(std::shared_ptr<dae::Texture>& pDiffuseTexture) {
	if (m_pDiffuseMapVariable) {
		m_pDiffuseMapVariable->SetResource(pDiffuseTexture->GetSRV());
	}
}

void BaseEffect::SetNormalMap(std::shared_ptr<dae::Texture>& pNormalTexture) {
	if (m_pNormalMapVariable) {
		m_pNormalMapVariable->SetResource(pNormalTexture->GetSRV());
	}
}

void BaseEffect::SetSpecularMap(std::shared_ptr<dae::Texture>& pSpecularTexture) {
	if (m_pSpecularMapVariable) {
		m_pSpecularMapVariable->SetResource(pSpecularTexture->GetSRV());
	}
}

void BaseEffect::SetGlossinessMap(std::shared_ptr<dae::Texture>& pGlossinessTexture) {
	if (m_pGlossinessMapVariable) {
		m_pGlossinessMapVariable->SetResource(pGlossinessTexture->GetSRV());
	}
}

void BaseEffect::SetCullMode(D3D11_CULL_MODE mode) {
	D3D11_RASTERIZER_DESC rasterizerDesc{};

	if (m_pRasterizerState) {
		m_pRasterizerState->GetDesc(&rasterizerDesc);
		m_pRasterizerState->Release();
	}

	rasterizerDesc.CullMode = mode;

	HRESULT hr{ m_pDevice->CreateRasterizerState(&rasterizerDesc, &m_pRasterizerState) };
	if (FAILED(hr)) {
		std::wcerr << L"Failed to create rasterizer state!\n";
		return;
	}

	hr = m_pRasterizerVariable->SetRasterizerState(0, m_pRasterizerState);
	if (FAILED(hr)) {
		std::cout << "Failed to change rasterizer state\n";
	}
}
