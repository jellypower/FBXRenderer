#include "SSModelAssetManager.h"

#include "SSEngineDefault/SSDebugLogger.h"
#include "AssetType/SSModelAsset.h"

#include "SSMaterialAssetManager.h"
#include "SSGeometryAssetManager.h"



SSModelAssetManager* SSModelAssetManager::g_instance = nullptr;

SSModelAssetManager::SSModelAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed)
	: _assetPool(poolCapacity), _assetHash(hashCapacity, hashCollisionLimit, hashSeed)
{
}

void SSModelAssetManager::InsertNewModel(SSModelAsset* newModelAsset)
{
	_assetPool.PushBack(newModelAsset);
	InsertResult result = _assetHash.TryInsert(newModelAsset->GetAssetName(), _assetPool.GetSize() - 1);
	SS_ASSERT(result == InsertResult::Success, "new asset insert failed");
}


void SSModelAssetManager::ReleaseAllModels()
{
	for(SSModelAsset* item : _assetPool)
	{
		delete item;
	}

}


SSModelAsset* SSModelAssetManager::FindModelWithNameInternal(const char* name)
{
	uint32 idxOut = SS_UINT32_MAX;
	if (_assetHash.TryFind(name, idxOut) == FindResult::Success)
		return _assetPool[idxOut];
	return nullptr;
}
