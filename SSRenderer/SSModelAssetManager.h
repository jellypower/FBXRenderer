#pragma once
#include "SSEngineDefault/SSNativeTypes.h"
#include "SSModelAsset.h"

class SSMaterialAssetManager;

class SSModelAssetManager
{
public:
	void Init(SSMaterialAssetManager* InMaterialManager);
	void Release();

	void ReleaseAllModels();
	
	void CreateNewAssetTemp(SSMaterialAsset* InMaterial, SSGeometryAsset* InGeometryAsset);

public:
	__forceinline SSModelAsset* GetModelWithIdx(uint32 idx) { return ModelAssetList[idx]; }

private:

	static constexpr uint8 DEFAULT_POOL_SIZE = 10;
	uint8 ModelAssetPoolCount = 0;
	SSModelAsset** ModelAssetList;

	SSMaterialAssetManager* MaterialManager;

};

