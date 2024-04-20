#pragma once
#include "SSEngineDefault/SSEngineDefault.h"
#include "AssetManagerContainer.h"

template<typename AssetType>
class SSAssetManagerBase
{
	friend class SSFBXImporter;
protected:

	SSAssetManagerBase(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed);
	AssetManagerContainer<AssetType*> _assetContainer;

public:
	virtual void ReleaseAllAssets();

	AssetType* FindAssetWithName(const char* name);
	AssetType* GetAsset(uint32 idx);

	void InsertNewAsset(AssetType* newAsset);
};

template<typename AssetType>
SSAssetManagerBase<AssetType>::SSAssetManagerBase(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed)
	: _assetContainer(poolCapacity, hashCapacity, hashCollisionLimit, hashSeed)
{ }

template<typename AssetType>
void SSAssetManagerBase<AssetType>::ReleaseAllAssets()
{
	_assetContainer.ReleaseAllAssets();
}

template<typename AssetType>
AssetType* SSAssetManagerBase<AssetType>::FindAssetWithName(const char* name)
{
	return _assetContainer.FindAssetWithName(name); 
}

template<typename AssetType>
AssetType* SSAssetManagerBase<AssetType>::GetAsset(uint32 idx)
{
	return _assetContainer.GetAssetWithIdx(idx);
}

template<typename AssetType>
void SSAssetManagerBase<AssetType>::InsertNewAsset(AssetType* newAsset)
{
	InsertResult result = _assetContainer.InsertNewAsset(newAsset);
}
