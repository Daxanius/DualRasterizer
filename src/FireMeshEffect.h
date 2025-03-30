#pragma once

#include "BaseEffect.h"

class FireMeshEffect final : public BaseEffect {
public:
	FireMeshEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
};