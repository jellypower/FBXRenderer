#pragma once
#include "SSEngineDefault/SSEngineDefault.h"
#include "SSEngineDefault/SSContainer/PooledList.h"
#include "SSEngineDefault/SSContainer/StringHashMapA.h"

template<typename AssetType>
class AssetManagerContainer
{

public:
	AssetManagerContainer(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit = COLLISION_LIMIT_DEFAULT, uint64 hashSeed = RANDOM_PRIMENO_FOR_HASH);

	InsertResult InsertNewAsset(AssetType NewAsset);
	AssetType FindAssetWithName(const char* name);
	AssetType GetAssetWithIdx(uint32 idx);

	void ReleaseAllAssets();

	SS::PooledList<AssetType> _list;
	SS::StringHashMapA<uint32> _hashMap;
};

template<typename AssetType>
AssetManagerContainer<AssetType>::AssetManagerContainer(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed)
	: _list(poolCapacity), _hashMap(hashCapacity, hashCollisionLimit, hashSeed)
{	}

template<typename AssetType>
inline InsertResult AssetManagerContainer<AssetType>::InsertNewAsset(AssetType NewAsset)
{
	if (_list.IsFull()) _list.IncreaseCapacityAndCopy(_list.GetCapacity() * 2);
	_list.PushBack(NewAsset);
	InsertResult result = _hashMap.TryInsert(NewAsset->GetAssetName(), _list.GetSize() - 1);

	if (result != InsertResult::Success)
	{
		SS::FixedStringA<300> ErrorString;
		ErrorString = "new asset insertion failed on hashmap. (asset name: ";
		ErrorString += NewAsset->GetAssetName();
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
	return result;
}

template<typename AssetType>
inline AssetType AssetManagerContainer<AssetType>::FindAssetWithName(const char* name)
{
	uint32 foundIdx = SS_UINT32_MAX;
	if (_hashMap.TryFind(name, foundIdx) == FindResult::Success)
		return _list[foundIdx];

	return nullptr;
}

template<typename AssetType>
inline AssetType AssetManagerContainer<AssetType>::GetAssetWithIdx(uint32 idx)
{
	return _list[idx];
}

template<typename AssetType>
inline void AssetManagerContainer<AssetType>::ReleaseAllAssets()
{
	for (AssetType assetItem : _list)
	{
		delete assetItem;
	}
}
