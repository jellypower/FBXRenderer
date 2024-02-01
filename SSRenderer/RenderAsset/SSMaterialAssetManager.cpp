#include "SSMaterialAssetManager.h"

#include "SSShaderAssetManager.h"
#include "SSTextureAssetManager.h"
#include "SSEngineDefault/SSDebugLogger.h"
#include "SSRenderer/SSSamplerPool.h"

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
	SSMaterialAsset* newMaterial = DBG_NEW SSMaterialAsset("SSDefaultPbr", SSShaderAssetManager::FindShaderWithName("SSDefaultPbr"));
	

	// HACK: create texture
	SSTextureAsset* colortTexture = SSTextureAssetManager::FindAssetWithName("rp_nathan_animated_003_dif");
	newMaterial->_textureCache[0] = colortTexture;

	// HACK: sampler
	{
		newMaterial->_sampleCache[0] = SSSamplerPool::Get()->GetSampler(TexFilterMode::Linear, TexAddressMode::Repeat, TexAddressMode::Repeat);
	}
	InsertNewMaterial(newMaterial);
}

void SSMaterialAssetManager::InstantiateAllMaterials(ID3D11Device* InDevice)
{
	for(SSMaterialAsset* materialItem :_assetPool)
	{
		materialItem->InstantiateSystemBuffer();
		materialItem->InstantiateGPUBuffer(InDevice);
	}
}

void SSMaterialAssetManager::ReleaseAllMaterials()
{
	for (SSMaterialAsset* item : _assetPool)
	{
		item->Release();
		delete item;
	}
}
