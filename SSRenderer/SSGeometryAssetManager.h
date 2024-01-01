#pragma once
#include "SSGeometryAsset.h"

#include <fbxsdk.h>

class SSGeometryAssetManager
{
public:
	void Init(/*TODO: file* GeometryList*/ int PoolSize = 0);
	void Release();

	void InstantiateNewGeometry(FbxMesh* InFbxMesh);

	HRESULT SendAllGeometryAssetToGPUTemp(ID3D11Device* InDevice);

	void ReleaseAllGeometryDataOnSystem();
	void ReleaseAllGeometryDataOnGPU();

public:
	__forceinline SSGeometryAsset* GetGeometryWithIdx(uint8 idx) { return GeometryList[idx]; }

private:

	static constexpr uint8 DEFAULT_POOL_SIZE = 10;
	uint8 GeometryPoolMax = 0;
	uint8 GeometryPoolCount = 0;
	SSGeometryAsset** GeometryList;
};

