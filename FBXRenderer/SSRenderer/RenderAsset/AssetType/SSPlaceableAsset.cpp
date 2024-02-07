#include "SSPlaceableAsset.h"

SSPlaceableAsset::SSPlaceableAsset(AssetType assetType, uint32 childCnt)
	: SSAssetBase(assetType), _childs(childCnt), _parent(nullptr), _transform()
{
}

SSPlaceableAsset::~SSPlaceableAsset()
{
	for (const SSPlaceableAsset* item : _childs)
	{
		delete item;
	}
}
