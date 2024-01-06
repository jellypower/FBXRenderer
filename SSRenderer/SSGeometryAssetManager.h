#pragma once
#include "SSEngineDefault/SSEngineDefault.h"
#include "SSGeometryAsset.h"

#include <fbxsdk.h>


class SSGeometryAssetManager
{
	friend class SSFBXImporter;
	friend class SSRenderer;
private:
	static SSGeometryAssetManager* _instance;

public:
	static FORCEINLINE SSGeometryAssetManager* Get() {
		assert(_instance != nullptr);
		return _instance;
	}
	static FORCEINLINE void Release() {
		delete _instance;
		_instance = nullptr;
	}
	static FORCEINLINE uint32 GetGeometryPoolCount() { return _instance->_geometryPoolCount; }
	static SSGeometryAsset* GetGeometryWithIdx(uint32 idx) { return _instance->GeometryList[idx]; }

private:
	static FORCEINLINE void Instantiate(uint32 poolSize) {
		if (_instance != nullptr) {
			assert(false);
			return;
		}
		_instance = DBG_NEW SSGeometryAssetManager(poolSize);
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

