#include "SSModelCombinationAsset.h"

#include <process.h>

#include "SSRenderer/RenderAsset/SSModelAssetManager.h"

SSModelCombinationAsset::SSModelCombinationAsset(const char* InAssetName, SSModelAsset* modelAsset, uint32 childCnt)
	:SSPlaceableAsset(AssetType::ModelCombination, childCnt)
{
	_assetName = InAssetName;
	_modelAsset = modelAsset;
}

SSModelCombinationAsset::SSModelCombinationAsset(const char* InAssetName, const char* modelName, uint32 childCnt)
	: SSPlaceableAsset(AssetType::ModelCombination, childCnt)
{
	_assetName = InAssetName;
	_modelAsset = SSModelAssetManager::FindModelWithName(modelName);
}

SSModelCombinationAsset::~SSModelCombinationAsset()
{
	__noop;
}
