#pragma once
#include "SSEngineDefault/SSEngineDefault.h"
#include "SSEngineDefault/SSContainer/PooledList.h"
#include "SSEngineDefault/SSContainer/StringHashMapA.h"
#include "AssetType/SSModelCombinationAsset.h"

class SSModelCombinationAssetManager
{
	SS_DECLARE_AS_SINGLETON(SSModelCombinationAssetManager)
public:
	static FORCEINLINE void Instantiate(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed) {
		if (g_instance != nullptr) {
			assert(false);
			return;
		}
		g_instance = DBG_NEW SSModelCombinationAssetManager(poolCapacity, hashCapacity, hashCollisionLimit, hashSeed);
	}
	static FORCEINLINE SSModelCombinationAsset* GetAssetWithIdx(uint32 idx) { return g_instance->_assetPool[idx]; }
	static FORCEINLINE SSModelCombinationAsset* FindAssetWithName(const char* name) { return g_instance->FindAssetWithNameInternal(name); }


private:
	SS::PooledList<SSModelCombinationAsset*> _assetPool;
	SS::StringHashMapA<uint32> _assetHash;

public:
	SSModelCombinationAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed);
	void InsertNewAsset(SSModelCombinationAsset* newModelCombination);

	void ReleaseAllAssets();

private:
	SSModelCombinationAsset* FindAssetWithNameInternal(const char* name);
};

