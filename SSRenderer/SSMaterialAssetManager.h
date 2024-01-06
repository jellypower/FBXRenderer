#pragma once

#include "SSEngineDefault/SSEngineDefault.h"
#include "SSMaterialAsset.h"


class SSMaterialAssetManager
{
	friend class SSFBXImporter;
	friend class SSRenderer;
	SS_DECLARE_AS_SINGLETON(SSMaterialAssetManager)
public:
	static FORCEINLINE uint32 GetPoolCount() { return g_instance->_poolCount; }
	static FORCEINLINE SSMaterialAsset* GetAsset(uint32 idx) { return g_instance->MaterialList[idx]; }

private:
	static FORCEINLINE void Instantiate(uint32 poolCapacity) {
		if (g_instance != nullptr) {
			assert(false);
			return;
		}
		g_instance = DBG_NEW SSMaterialAssetManager(poolCapacity);
	}



private:
	uint32 _poolCount = 0;
	uint32 _poolCapacity = 0;
	SSMaterialAsset** MaterialList;

public:
	FORCEINLINE SSMaterialAsset* GetMaterialWithIdx(uint32 idx) { return MaterialList[idx]; }

private:
	SSMaterialAssetManager(uint32 poolSize);
	~SSMaterialAssetManager();

	HRESULT InstantiateAllMaterialsTemp(ID3D11Device* InDevice);
	void ReleaseAllMaterialsTemp();
};

