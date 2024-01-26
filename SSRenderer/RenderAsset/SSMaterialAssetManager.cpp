#include "SSMaterialAssetManager.h"
#include "SSShaderAssetManager.h"
#include "SSTextureAssetManager.h"
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

	// HACK: create texture
	SSTextureAsset* colortTexture = SSTextureAssetManager::FindAssetWithName("Worm_SSS_Color");
	newMaterial->_textureList[0] = colortTexture;

	// HACK: create sampler
	{
		ID3D11SamplerState* sampler;
		D3D11_SAMPLER_DESC sampDesc = {};
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

		HRESULT hr = InDevice->CreateSamplerState(&sampDesc, &sampler);
		assert(SUCCEEDED(hr));
		newMaterial->_sampleStateList[0] = sampler;

	}
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
