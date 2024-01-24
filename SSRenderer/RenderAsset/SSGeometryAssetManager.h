#pragma once
#include "SSEngineDefault/SSEngineDefault.h"
#include "AssetType/SSGeometryAsset.h"

#include <fbxsdk.h>

#include "SSEngineDefault/SSContainer/PooledList.h"
#include "SSEngineDefault/SSContainer/StringHashMapA.h"


class SSGeometryAssetManager
{
	friend class SSFBXImporter;
	friend class SSRenderer;
	SS_DECLARE_AS_SINGLETON(SSGeometryAssetManager);

public:
	static FORCEINLINE uint32 GetGeometryPoolCount() { return g_instance->_assetPool.GetCapacity(); }
	static FORCEINLINE SSGeometryAsset* GetGeometryWithIdx(uint32 idx) { return g_instance->_assetPool[idx]; }
	static FORCEINLINE SSGeometryAsset* FindGeometryWithName(const char* name) { return g_instance->FindGeometryWithNameInternal(name); }
	
private:
	static FORCEINLINE void Instantiate(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed) {
		if (g_instance != nullptr) {
			assert(false);
			return;
		}
		g_instance = DBG_NEW SSGeometryAssetManager(poolCapacity, hashCapacity, hashCollisionLimit, hashSeed);
	}





private:
	SS::PooledList<SSGeometryAsset*> _assetPool;
	SS::StringHashMapA<uint32, ASSET_NAME_LEN_MAX> _assetHashmap;

private:
	SSGeometryAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed);
	~SSGeometryAssetManager();

	SSGeometryAsset* FindGeometryWithNameInternal(const char* name);

	void InsertNewGeometry(SSGeometryAsset* newGeometry);

	HRESULT SendAllGeometryAssetToGPUTemp(ID3D11Device* InDevice);

	void ReleaseAllGeometryDataOnSystem();
	void ReleaseAllGeometryDataOnGPU();

};

