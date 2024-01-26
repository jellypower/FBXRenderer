#include "SSTextureAsset.h"

#include "ExternalUtils/DDSTextureLoader.h"
#include "ExternalUtils/ExternalUtils.h"

SSTextureAsset::SSTextureAsset(const char* InAssetName, const char* InAssetPath) :
	SSAssetBase(AssetType::Texture)
{
	_assetPath = InAssetPath;
	_assetName = InAssetName;
}

HRESULT SSTextureAsset::UpdateOnGPU(ID3D11Device* InDevice)
{
	HRESULT hr = S_OK;
	hr = DirectX::CreateDDSTextureFromFile(InDevice, _assetPath, &_shaderRes, &_shaderRV);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("Textrue creation failed.");
		return hr;
	}

	return hr;
}

void SSTextureAsset::ReleaseGPUData()
{
	if (_shaderRes != nullptr) {
		_shaderRes->Release();
		_shaderRV->Release();
		_shaderRes = nullptr;
		_shaderRV = nullptr;
	}
}

bool SSTextureAsset::UsableOnGPU() const
{
	return _shaderRV != nullptr;
}
