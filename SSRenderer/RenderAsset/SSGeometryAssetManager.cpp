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
	WASSERT_WITH_MESSAGE(result == InsertResult::Success, "FBX Asset's object names must be unique");
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
