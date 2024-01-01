#include "SSMaterialAssetManager.h"
#include "SSShaderAssetManager.h"
#include "SSEngineDefault/SSDebugLogger.h"
#include "SSTextureManager.h"


void SSMaterialAssetManager::Init()
{
	MaterialList = DBG_NEW SSMaterialAsset * [DEFAULT_POOL_SIZE];
}

void SSMaterialAssetManager::Release()
{
	delete[] MaterialList;
}

HRESULT SSMaterialAssetManager::InstantiateAllMaterialsTemp(ID3D11Device* InDevice
	, SSShaderAssetManager* InShaderManager, SSTextureManager* InTextureManager)
{
	MaterialList[MaterialPoolCount++] = DBG_NEW SSMaterialAsset();
	HRESULT hr = MaterialList[0]->InitTemp(InDevice
		, InShaderManager->GetShaderAsset(0), InTextureManager);

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
	for (int i = 0; i < MaterialPoolCount; i++) {
		MaterialList[i]->Release();
		delete MaterialList[i];
	}


}
