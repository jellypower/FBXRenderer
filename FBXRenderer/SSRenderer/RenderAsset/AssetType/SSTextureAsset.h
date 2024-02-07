#pragma once
#include <d3d11.h>

#include "SSAssetBase.h"

class SSTextureAsset : public SSAssetBase
{
private:
	
	ID3D11ShaderResourceView* _shaderRV = nullptr;
	ID3D11Resource* _shaderRes = nullptr;

public:
	SSTextureAsset(const char* InAssetName, const char* InAssetPath);

	ID3D11ShaderResourceView** const GetSRVpp() { return &_shaderRV; }

	HRESULT UpdateOnGPU(ID3D11Device* InDevice);
	void ReleaseGPUData();
	bool UsableOnGPU() const;
};

