#pragma once
#include "SSEngineDefault/SSEngineDefault.h"
#include "SSAssetManagerBase.h"
#include "AssetType/SSSkeletonAnimAsset.h"

class SSSkeletonAnimAssetManager : public SSAssetManagerBase<SSSkeletonAnimAsset>
{
public:
	static SSSkeletonAnimAssetManager* g_instance;

	static FORCEINLINE SSSkeletonAnimAssetManager* Get() {
		assert(g_instance != nullptr);
		return g_instance;
	}
	static FORCEINLINE void Release() {
		delete g_instance;
		g_instance = nullptr;
	}
	static FORCEINLINE void Instantiate(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed)
	{
		if (g_instance != nullptr) {
			assert(false);
			return;
		}
		g_instance = DBG_NEW SSSkeletonAnimAssetManager(poolCapacity, hashCapacity, hashCollisionLimit, hashSeed);
	}

	void InstantiateAllAsets(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext);


protected:
	SSSkeletonAnimAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed);
	~SSSkeletonAnimAssetManager();
};


