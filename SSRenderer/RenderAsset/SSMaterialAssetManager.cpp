#include "SSMaterialAssetManager.h"
#include "SSShaderAssetManager.h"
#include "SSEngineDefault/SSDebugLogger.h"

SSMaterialAssetManager* SSMaterialAssetManager::g_instance = nullptr;


SSMaterialAsset* SSMaterialAssetManager::FindMaterialWithNameInternal(const char* name)
{
	uint32 foundIdx = SS_UINT32_MAX;
	if (_assetHash.TryFind(name, foundIdx) == FindResult::Success)
		return _assetPool[foundIdx];

	return nullptr;
}

SSMaterialAssetManager::SSMaterialAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed)
	: _assetPool(poolCapacity), _assetHash(hashCapacity, hashCollisionLimit, hashSeed)
{
}

SSMaterialAssetManager::~SSMaterialAssetManager()
{

}

void SSMaterialAssetManager::InsertNewMaterial(SSMaterialAsset* newMaterialAsset)
{
	_assetPool.PushBack(newMaterialAsset);
	InsertResult result = _assetHash.TryInsert(newMaterialAsset->GetAssetName(), _assetPool.GetSize() - 1);
	WASSERT_WITH_MESSAGE(result == InsertResult::Success, "new material insertion failed");
}

void SSMaterialAssetManager::CreateTempMaterials(ID3D11Device* InDevice)
{
	SSMaterialAsset* newMaterial = DBG_NEW SSMaterialAsset("Phong", SSShaderAssetManager::FindShaderWithName("Phong"));
	newMaterial->InstantiateGPUBuffer(InDevice);

	InsertNewMaterial(newMaterial);
}

void SSMaterialAssetManager::ReleaseAllMaterialsTemp()
{
	for (SSMaterialAsset* item : _assetPool)
	{
		item->Release();
		delete item;
	}
}
