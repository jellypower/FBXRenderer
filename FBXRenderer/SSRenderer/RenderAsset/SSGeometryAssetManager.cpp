#include "SSGeometryAssetManager.h"
#include "SSEngineDefault/SSDebugLogger.h"

SSGeometryAssetManager* SSGeometryAssetManager::g_instance = nullptr;

SSGeometryAssetManager::SSGeometryAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit,
	uint64 hashSeed)
	: _assetPool(poolCapacity), _assetHashmap(hashCapacity, hashCollisionLimit, hashSeed)
{
	__noop;
}

SSGeometryAssetManager::~SSGeometryAssetManager()
{
	for (const SSGeometryAsset* item : _assetPool)
		delete item;
}

SSGeometryAsset* SSGeometryAssetManager::FindGeometryWithNameInternal(const char* name)
{
	uint32 outResult;
	if (_assetHashmap.TryFind(name, outResult) == FindResult::Success)
		return _assetPool[outResult];

	return nullptr;
}

void SSGeometryAssetManager::InsertNewGeometry(SSGeometryAsset* newGeometry)
{
	_assetPool.PushBack(newGeometry);
	InsertResult result = _assetHashmap.TryInsert(newGeometry->GetAssetName(), _assetPool.GetSize() - 1);

	if(result != InsertResult::Success)
	{
		SS::FixedStringA<300> ErrorString;
		ErrorString = "new geometry insertion failed on hashmap. (geometry name: ";
		ErrorString += newGeometry->GetAssetName();
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

HRESULT SSGeometryAssetManager::SendAllGeometryAssetToGPUTemp(ID3D11Device* InDevice)
{
	for (SSGeometryAsset* asset : _assetPool)
	{
		HRESULT hr = asset->UpdateDataOnGPU(InDevice);

		if (FAILED(hr))
		{
			SS_CLASS_ERR_LOG("Geometry data send to gpu failed. (AssetName: %s)", asset->GetAssetName());
			return hr;
		}
	}

	return S_OK;
}

void SSGeometryAssetManager::ReleaseAllGeometryDataOnSystem()
{
	for (SSGeometryAsset* asset : _assetPool)
	{
		asset->ReleaseSystemData();
	}
}

void SSGeometryAssetManager::ReleaseAllGeometryDataOnGPU()
{
	for (SSGeometryAsset* asset : _assetPool)
	{
		asset->ReleaseGPUData();
	}
}
