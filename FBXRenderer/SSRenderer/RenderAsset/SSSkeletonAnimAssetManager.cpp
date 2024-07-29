#include "SSSkeletonAnimAssetManager.h"

SSSkeletonAnimAssetManager* SSSkeletonAnimAssetManager::g_instance = nullptr;

void SSSkeletonAnimAssetManager::InstantiateAllAsets(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext)
{
	for (SSSkeletonAnimAsset* asset : _assetContainer._list)
	{
		asset->InstantiateGPUBuffer(InDevice, InDeviceContext);
	}
}

void SSSkeletonAnimAssetManager::ReleaseAllAssets()
{
	for (SSSkeletonAnimAsset* asset : _assetContainer._list)
	{
		asset->ReleaseGPUBuffer();
		delete asset;
	}
}

SSSkeletonAnimAssetManager::SSSkeletonAnimAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed)
	: SSAssetManagerBase<SSSkeletonAnimAsset>(poolCapacity, hashCapacity, hashCollisionLimit, hashSeed)
{ }

SSSkeletonAnimAssetManager::~SSSkeletonAnimAssetManager()
{ }

