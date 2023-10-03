#include "SSModelAssetManager.h"

#include "SSDebugLogger.h"
#include "SSModelAsset.h"

void SSModelAssetManager::Init()
{
	ModelAssetList = new SSModelAsset * [DEFAULT_POOL_SIZE];
}

void SSModelAssetManager::Release()
{
	delete[] ModelAssetList;
}

