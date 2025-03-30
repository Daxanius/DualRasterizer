#pragma once

#include "pch.h"

#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>
#include "Texture.h"
#include "Vector3.h"
#include "Matrix.h"

// An abstract effect class that implements some of the common functionalities between effects
class BaseEffect {
public:
	enum class TechniqueType {
		Point,
		Linear,
		Anisotropic
	};

	virtual ~BaseEffect();

	// Effects should not be copied
	BaseEffect(const BaseEffect&) = delete;
	BaseEffect& operator=(const BaseEffect&) = delete;
	BaseEffect(BaseEffect&&) noexcept = default;
	BaseEffect& operator=(BaseEffect&&) noexcept = default;

	ID3D11InputLayout* GetInputLayout() const;

	void SetWorldMatrixVariable(dae::Matrix& matrix);
	void SetCameraPositionVariable(dae::Vector3& position);
	void SetWorldViewProjectionVariable(dae::Matrix& matrix);
	void SetDiffuseMap(std::shared_ptr<dae::Texture>& pDiffuseTexture);
	void SetNormalMap(std::shared_ptr<dae::Texture>& pNormalTexture);
	void SetSpecularMap(std::shared_ptr<dae::Texture>& pSpecularTexture);
	void SetGlossinessMap(std::shared_ptr<dae::Texture>& pGlossinessTexture);

	void SetCullMode(D3D11_CULL_MODE mode);

	ID3DX11Effect* GetEffect() const;
	ID3DX11EffectTechnique* GetTechnique(TechniqueType type) const;
protected:
	BaseEffect(ID3D11Device* pDevice, const std::wstring& assetFile);

	ID3D11Device* m_pDevice{ nullptr };

	ID3DX11Effect* m_pEffect{ nullptr };

	ID3D11RasterizerState* m_pRasterizerState{ nullptr };
	ID3DX11EffectRasterizerVariable* m_pRasterizerVariable{ nullptr };

	ID3DX11EffectTechnique* m_pPointTechnique{ nullptr };
	ID3DX11EffectTechnique* m_pLinearTechnique{ nullptr };
	ID3DX11EffectTechnique* m_pAnisotropicTechnique{ nullptr };

	ID3DX11EffectMatrixVariable* m_pMatWorldViewProjVariable{ nullptr };

	ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable{ nullptr };
	ID3DX11EffectShaderResourceVariable* m_pNormalMapVariable{ nullptr };
	ID3DX11EffectShaderResourceVariable* m_pSpecularMapVariable{ nullptr };
	ID3DX11EffectShaderResourceVariable* m_pGlossinessMapVariable{ nullptr };

	ID3DX11EffectVectorVariable* m_pCameraPositionVariable{ nullptr };
	ID3DX11EffectMatrixVariable* m_pWorldMatrixVariable{ nullptr };
	
	ID3D11InputLayout* m_pInputLayout{ nullptr };
private:
	static ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
};