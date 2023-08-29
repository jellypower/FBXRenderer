#include "SSMaterialAssetManager.h"
#include "SSShaderAssetManager.h"
#include "SSDebug.h"
#include "SSTextureManager.h"


void SSMaterialAssetManager::Init()
{
	MaterialList = new SSMaterial * [DEFAULT_POOL_SIZE];
}

void SSMaterialAssetManager::Release()
{
	delete[] MaterialList;
}

HRESULT SSMaterialAssetManager::InstantiateAllMaterialsTemp(ID3D11Device* InDevice
	, SSShaderAssetManager* InShaderManager, SSTextureManager* InTextureManager)
{
	MaterialList[MaterialPoolCount++] = new SSMaterial();
	HRESULT hr = MaterialList[0]->InitTemp(InDevice
		, InShaderManager->GetShaderAsset(0), InTextureManager);

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
