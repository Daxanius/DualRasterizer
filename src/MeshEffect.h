#pragma once

#include "pch.h"

#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>
#include "Texture.h"
#include "Vector3.h"
#include "BaseEffect.h"

class MeshEffect final : public BaseEffect {
public:
	MeshEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
};