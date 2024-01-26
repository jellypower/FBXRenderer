#include "SSTextureAssetManager.h"

#include "SSEngineDefault/SSDebugLogger.h"
#include "ExternalUtils/DDSTextureLoader.h"

using namespace DirectX;

SSTextureAssetManager* SSTextureAssetManager::g_instance = nullptr;

SSTextureAssetManager::SSTextureAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed):
	_textureList(poolCapacity),
	_assetHashmap(hashCapacity, hashCollisionLimit, hashSeed)
{ }

SSTextureAssetManager::~SSTextureAssetManager()
{ }

SSTextureAsset* SSTextureAssetManager::FindAssetWithNameInternal(const char* name)
{
	uint32 idxOut = SS_UINT32_MAX;
	if (_assetHashmap.TryFind(name, idxOut) == FindResult::Success)
		return _textureList[idxOut];
	return nullptr;
}



HRESULT SSTextureAssetManager::TempLoadTexture(ID3D11Device* InDevice)
{
	InsertNewAsset(DBG_NEW SSTextureAsset("Teeth_Bump", "Resource/Texture/Teeth_Bump.dds"));
	InsertNewAsset(DBG_NEW SSTextureAsset("Teeth_reflection", "Resource/Texture/Teeth_reflection.dds"));
	InsertNewAsset(DBG_NEW SSTextureAsset("Teeth_SSS_Color", "Resource/Texture/Teeth_SSS_Color.dds"));
	InsertNewAsset(DBG_NEW SSTextureAsset("Worm_Bump", "Resource/Texture/Worm_Bump.dds"));
	InsertNewAsset(DBG_NEW SSTextureAsset("Worm_reflection", "Resource/Texture/Worm_reflection.dds"));
	InsertNewAsset(DBG_NEW SSTextureAsset("Worm_SSS_Color", "Resource/Texture/Worm_SSS_Color.dds"));

	HRESULT hr = S_OK;
	for(SSTextureAsset* textureItem : _textureList)
	{
		hr = textureItem->UpdateOnGPU(InDevice);
		if(FAILED(hr))
		{
			SS_CLASS_ERR_LOG("Texture upload failed. Texture Name: %s", textureItem->GetAssetName());
			return hr;
		}
	}
	return hr;
}


void SSTextureAssetManager::ReleaseAllTextures()
{
	for (int i = 0; i < _textureList.GetSize(); i++) {
		_textureList[i]->ReleaseGPUData();
		delete _textureList[i];
;	}

}

void SSTextureAssetManager::InsertNewAsset(SSTextureAsset* newTextureAsset)
{
	_textureList.PushBack(newTextureAsset);
	InsertResult result = _assetHashmap.TryInsert(newTextureAsset->GetAssetName(), _textureList.GetSize() - 1);
	SS_ASSERT(result == InsertResult::Success, "new asset insert failed");
}

