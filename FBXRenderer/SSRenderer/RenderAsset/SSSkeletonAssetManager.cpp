#include "SSSkeletonAssetManager.h"

SSSkeletonAssetManager* SSSkeletonAssetManager::g_instance = nullptr;

SSSkeletonAssetManager::SSSkeletonAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed)
	: _assetPool(poolCapacity), _assetHashMap(hashCapacity, hashCollisionLimit, hashSeed)
{
}

SSSkeletonAssetManager::~SSSkeletonAssetManager()
{

}

void SSSkeletonAssetManager::InsertNewAsset(SSSkeletonAsset* newSkeleton)
{
	if (_assetPool.IsFull()) _assetPool.IncreaseCapacityAndCopy(_assetPool.GetCapacity() * 2);
	_assetPool.PushBack(newSkeleton);
	InsertResult result = _assetHashMap.TryInsert(newSkeleton->GetAssetName(), _assetPool.GetSize() - 1);

	if (result != InsertResult::Success)
	{
		SS::FixedStringA<300> ErrorString;
		ErrorString = "new material insertion failed on hashmap. (material name: ";
		ErrorString += newSkeleton->GetAssetName();
		ErrorString += "), (ErrorCode: ";

		switch (result)
		{
		case InsertResult::CollisionLimit: ErrorString += "CollisionLimit)";
		case InsertResult::InvalidStrKey: ErrorString += "InvalidStrKey)";
		case InsertResult::KeyAlreadyExist: ErrorString += "KeyAlreadyExist)";
		}

		ASSERT_WITH_MESSAGE(result == InsertResult::Success, ErrorString);
		MessageBoxA(nullptr, ErrorString, "Alert", MB_OK);
	}
}

void SSSkeletonAssetManager::InstantiateAllSkeletonGPUBuffer(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext)
{
	for (SSSkeletonAsset* skeletonItem : _assetPool)
	{
		skeletonItem->InstantiateGPUBuffer(InDevice, InDeviceContext);
	}
}

void SSSkeletonAssetManager::ReleaseAllSkeletons()
{
	for (SSSkeletonAsset* skeletonItem : _assetPool)
	{
		skeletonItem->ReleaseGPUBuffer();
		delete skeletonItem;
	}
}

SSSkeletonAsset* SSSkeletonAssetManager::FindAssetWithNameInternal(const char* name)
{
	uint32 foundIdx = SS_UINT32_MAX;
	if (_assetHashMap.TryFind(name, foundIdx) == FindResult::Success)
		return _assetPool[foundIdx];

	return nullptr;
}
