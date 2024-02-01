#pragma once

#include "SSEngineDefault/SSEngineDefault.h"
#include "SSEngineDefault/SSContainer/PooledList.h"
#include "AssetType/SSMaterialAsset.h"
#include "SSEngineDefault/SSContainer/StringHashMapA.h"

#include "Serializable/SSPbrMaterialData.capnp.h"

class SSMaterialAssetManager
{
	friend class SSFBXImporter;
	friend class SSRenderer;
	SS_DECLARE_AS_SINGLETON(SSMaterialAssetManager)
public:
	static FORCEINLINE uint32 GetPoolCapacity() { return g_instance->_assetPool.GetCapacity(); }
	static FORCEINLINE SSMaterialAsset* GetAssetWithIdx(uint32 idx) { return g_instance->GetMaterialWithIdxInternal(idx); }
	static FORCEINLINE SSMaterialAsset* FindAssetWithName(const char* name) { return g_instance->FindMaterialWithNameInternal(name); }
	static FORCEINLINE SSMaterialAsset* GetEmptyAsset() { return g_instance->GetMaterialWithIdxInternal(0); }

private:
	static FORCEINLINE void Instantiate(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed) {
		if (g_instance != nullptr) {
			assert(false);
			return;
		}
		g_instance = DBG_NEW SSMaterialAssetManager(poolCapacity, hashCapacity, hashCollisionLimit, hashSeed);
	}



private:
	SS::PooledList<SSMaterialAsset*> _assetPool;
	SS::StringHashMapA<uint32> _assetHash;


private:
	FORCEINLINE SSMaterialAsset* GetMaterialWithIdxInternal(uint32 idx) { return _assetPool[idx]; }
	SSMaterialAsset* FindMaterialWithNameInternal(const char* name);
	

private:
	SSMaterialAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed);
	~SSMaterialAssetManager();

	void InsertNewMaterial(SSMaterialAsset* newMaterialAsset);

	void CreateTempMaterials(ID3D11Device* InDevice);
	void InstantiateAllMaterials(ID3D11Device* InDevice);
	void ReleaseAllMaterials();
};

