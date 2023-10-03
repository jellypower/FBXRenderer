#pragma once
#include "SSNativeTypes.h"

class SSModelAsset;

class SSModelAssetManager
{
public:
	void Init(/*TODO: file* ModelList*/);
	void Release();


public:
	__forceinline SSModelAsset* GetModelWithIdx(uint32 idx) { return ModelAssetList[idx]; }

private:

	static constexpr uint8 DEFAULT_POOL_SIZE = 10;
	uint8 ModelAssetPoolCount = 0;
	SSModelAsset** ModelAssetList;

};

