#pragma once

#include "SSNativeTypes.h"
#include "SSMaterial.h"

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
	__forceinline SSMaterial* GetMaterialWithIdx(uint8 idx) { return MaterialList[idx]; }

private:

	static constexpr uint8 DEFAULT_POOL_SIZE = 10;
	uint8 MaterialPoolCount = 0;
	SSMaterial** MaterialList;


};

