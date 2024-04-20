#pragma once
#include "SSEngineDefault/SSEngineDefault.h"
#include "AssetType/SSSkeletonAsset.h"
#include "SSEngineDefault/SSContainer/PooledList.h"
#include "SSEngineDefault/SSContainer/StringHashMapA.h"


class SSSkeletonAssetManager
{
	SS_DECLARE_AS_SINGLETON(SSSkeletonAssetManager)
public:
	static FORCEINLINE void Instantiate(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed) {
		if (g_instance != nullptr) {
			assert(false);
			return;
		}
		g_instance = DBG_NEW SSSkeletonAssetManager(poolCapacity, hashCapacity, hashCollisionLimit, hashSeed);
	}
	static FORCEINLINE SSSkeletonAsset* FindAssetWithName(const char* name) { return g_instance->FindAssetWithNameInternal(name); }
	static FORCEINLINE SSSkeletonAsset* GetAsset(uint32 idx) { return g_instance->_assetPool[idx]; }

private:
	SS::PooledList<SSSkeletonAsset*> _assetPool;
	SS::StringHashMapA<uint32> _assetHashMap;

private:
	SSSkeletonAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed);
	~SSSkeletonAssetManager();


public:

	void InsertNewAsset(SSSkeletonAsset* newSkeleton);
	void InstantiateAllSkeletonGPUBuffer(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext);
	void ReleaseAllSkeletons();

private:
	SSSkeletonAsset* FindAssetWithNameInternal(const char* name);
};

