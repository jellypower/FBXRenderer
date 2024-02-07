#include "SSModelCombinationAssetManager.h"

SSModelCombinationAssetManager* SSModelCombinationAssetManager::g_instance = nullptr;

SSModelCombinationAssetManager::SSModelCombinationAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed)
	: _assetPool(poolCapacity), _assetHash(hashCapacity, hashCollisionLimit, hashSeed)
{
}

void SSModelCombinationAssetManager::InsertNewAsset(SSModelCombinationAsset* newModelCombination)
{
	_assetPool.PushBack(newModelCombination);
	InsertResult result = _assetHash.TryInsert(newModelCombination->GetAssetName(), _assetPool.GetSize() - 1);
	SS_ASSERT(result == InsertResult::Success, "new asset insert failed");
}

void SSModelCombinationAssetManager::ReleaseAllAssets()
{
	for (SSModelCombinationAsset* item : _assetPool)
	{
		delete item;
	}
}

SSModelCombinationAsset* SSModelCombinationAssetManager::FindAssetWithNameInternal(const char* name)
{
	uint32 idxOut = SS_UINT32_MAX;
	if (_assetHash.TryFind(name, idxOut) == FindResult::Success)
		return _assetPool[idxOut];
	return nullptr;
}
