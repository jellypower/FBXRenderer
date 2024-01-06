#include "SSModelAssetManager.h"

#include "SSEngineDefault/SSDebugLogger.h"
#include "SSModelAsset.h"

#include "SSRenderer/SSMaterialAssetManager.h"
#include "SSRenderer/SSGeometryAssetManager.h"

void SSModelAssetManager::Init(SSMaterialAssetManager* InMaterialManager)
{
	ModelAssetList = DBG_NEW SSModelAsset * [DEFAULT_POOL_SIZE];
	MaterialManager = InMaterialManager;
}


void SSModelAssetManager::ReleaseAllModels()
{
	for (int i = 0; i < ModelAssetPoolCount; i++) {
		ModelAssetList[i]->Release();
		delete ModelAssetList[i];
	}

}


void SSModelAssetManager::Release()
{
	MaterialManager = nullptr;
	delete[] ModelAssetList;
}

void SSModelAssetManager::CreateNewAssetTemp(SSMaterialAsset* InMaterial, SSGeometryAsset* InGeometryAsset)
{
	SSModelAsset* tempAsset;
	ModelAssetList[ModelAssetPoolCount++] = tempAsset = DBG_NEW SSModelAsset();
	tempAsset->InitTemp(InMaterial, InGeometryAsset);
}