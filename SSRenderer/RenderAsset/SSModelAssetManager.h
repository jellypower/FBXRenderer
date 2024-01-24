#pragma once
#include "SSEngineDefault/SSNativeTypes.h"
#include "SSEngineDefault/SSContainer/PooledList.h"

#include "AssetType/SSModelAsset.h"
#include "SSEngineDefault/SSContainer/StringHashMapA.h"


class SSModelAssetManager
{
	SS_DECLARE_AS_SINGLETON(SSModelAssetManager);
public:
	static FORCEINLINE void Instantiate(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed) {
		if (g_instance != nullptr) {
			assert(false);
			return;
		}
		g_instance = DBG_NEW SSModelAssetManager(poolCapacity, hashCapacity, hashCollisionLimit, hashSeed);
	}
	static FORCEINLINE SSModelAsset* GetAssetWithIdx(uint32 idx) { return g_instance->_assetPool[idx]; }
	static FORCEINLINE SSModelAsset* FindModelWithName(const char* name) { return g_instance->FindModelWithNameInternal(name); }



private:
	SS::PooledList<SSModelAsset*> _assetPool;
	SS::StringHashMapA<uint32> _assetHash;

public:
	SSModelAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed);
	void InsertNewModel(SSModelAsset* newModelAsset);

	void ReleaseAllModels();
	

private:
	FORCEINLINE SSModelAsset* GetModelWithIdxInternal(uint32 idx) { return _assetPool[idx]; }
	SSModelAsset* FindModelWithNameInternal(const char* name);

};

