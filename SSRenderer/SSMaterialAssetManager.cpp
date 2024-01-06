#include "SSMaterialAssetManager.h"
#include "SSShaderAssetManager.h"
#include "SSEngineDefault/SSDebugLogger.h"

#include "SSShaderAssetManager.h"
#include "SSTextureManager.h"

SSMaterialAssetManager* SSMaterialAssetManager::g_instance = nullptr;

SSMaterialAssetManager::SSMaterialAssetManager(uint32 poolCapacity)
{
	MaterialList = DBG_NEW SSMaterialAsset * [poolCapacity];
	_poolCapacity = poolCapacity;
}

SSMaterialAssetManager::~SSMaterialAssetManager()
{
	delete[] MaterialList;
}

HRESULT SSMaterialAssetManager::InstantiateAllMaterialsTemp(ID3D11Device* InDevice)
{
	MaterialList[_poolCount++] = DBG_NEW SSMaterialAsset();
	HRESULT hr = MaterialList[0]->InitTemp(InDevice
		, SSShaderAssetManager::Get()->GetShaderAsset(0));

	hr = MaterialList[0]->InstantiateShader(InDevice);

	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("material instantiate failed.");
		delete MaterialList[0];
		return hr;
	}

	return hr;

}

void SSMaterialAssetManager::ReleaseAllMaterialsTemp()
{
	for (int i = 0; i < _poolCount; i++) {
		MaterialList[i]->Release();
		delete MaterialList[i];
	}
}
