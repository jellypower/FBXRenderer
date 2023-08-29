#pragma once
#include "SSGeometryAsset.h"

class SSGeometryAssetManager
{
public:
	void Init(/*TODO: file* GeometryList*/);
	void Release();

	void LoadAllGeometryAssetTemp();
	HRESULT SendAllGeometryAssetToGPUTemp(ID3D11Device* InDevice);

	void ReleaseAllGeometryDataOnSystem();
	void ReleaseAllGeometryDataOnGPU();

public:
	__forceinline SSGeometryAsset* GetGeometryWithIdx(uint8 idx) { return GeometryList[idx]; }

private:

	static constexpr uint8 DEFAULT_POOL_SIZE = 10;
	uint8 GeometryPoolCount = 0;
	SSGeometryAsset** GeometryList;
};

