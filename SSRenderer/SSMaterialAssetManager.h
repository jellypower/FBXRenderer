#pragma once

#include "SSEngineDefault/SSNativeTypes.h"
#include "SSMaterialAsset.h"

class SSShaderAssetManager;

class SSMaterialAssetManager
{

public:
	
	void Init(/*TODO: file* ShaderList*/);
	void Release();
	HRESULT InstantiateAllMaterialsTemp(ID3D11Device* InDevice
		, SSShaderAssetManager* InShaderManager, class SSTextureManager* InTextureManager);
	void ReleaseAllMaterialsTemp();
	
public:
	__forceinline SSMaterialAsset* GetMaterialWithIdx(uint32 idx) { return MaterialList[idx]; }

private:

	static constexpr uint8 DEFAULT_POOL_SIZE = 10;
	uint8 MaterialPoolCount = 0;
	SSMaterialAsset** MaterialList;


};

