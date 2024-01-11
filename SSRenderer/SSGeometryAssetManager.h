#pragma once
#include "SSEngineDefault/SSEngineDefault.h"
#include "SSGeometryAsset.h"

#include <fbxsdk.h>


class SSGeometryAssetManager
{
	friend class SSFBXImporter;
	friend class SSRenderer;
private:
	SS_DECLARE_AS_SINGLETON(SSGeometryAssetManager);

public:
	static FORCEINLINE uint32 GetGeometryPoolCount() { return g_instance->_geometryPoolCount; }
	static SSGeometryAsset* GetGeometryWithIdx(uint32 idx) { return g_instance->GeometryList[idx]; }

private:
	static FORCEINLINE void Instantiate(uint32 poolSize) {
		if (g_instance != nullptr) {
			assert(false);
			return;
		}
		g_instance = DBG_NEW SSGeometryAssetManager(poolSize);
	}





private:
	uint32 _geometryPoolCapacity = 0;
	uint32 _geometryPoolCount = 0;
	SSGeometryAsset** GeometryList;

private:
	SSGeometryAssetManager(uint32 poolSize);
	~SSGeometryAssetManager();

	void InstantiateNewGeometry(FbxMesh* InFbxMesh);

	HRESULT SendAllGeometryAssetToGPUTemp(ID3D11Device* InDevice);

	void ReleaseAllGeometryDataOnSystem();
	void ReleaseAllGeometryDataOnGPU();
};

