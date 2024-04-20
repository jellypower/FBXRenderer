#include "SSTextureAssetManager.h"

#include "capnp/serialize-packed.h"
#include "capnp/compat/json.h"
#include "SSEngineDefault/SSDebugLogger.h"
#include "ExternalUtils/DDSTextureLoader.h"
#include "Serializable/SSTextureAssetManagingList.capnp.h"

using namespace DirectX;

SSTextureAssetManager* SSTextureAssetManager::g_instance = nullptr;

SSTextureAssetManager::SSTextureAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed):
	_textureList(poolCapacity),
	_assetHashmap(hashCapacity, hashCollisionLimit, hashSeed)
{ }

SSTextureAssetManager::~SSTextureAssetManager()
{ }

SSTextureAsset* SSTextureAssetManager::FindAssetWithNameInternal(const char* name, TexToReturnOnFail OnFailReturn)
{
	uint32 idxOut = SS_UINT32_MAX;
	if (_assetHashmap.TryFind(name, idxOut) == FindResult::Success)
		return _textureList[idxOut];

	if (OnFailReturn == TexToReturnOnFail::Return_NULL) return nullptr;

	return _textureList[static_cast<uint32>(OnFailReturn)];
}



HRESULT SSTextureAssetManager::LoadTextures(ID3D11Device* InDevice, const utf16* TextureAssetListPath)
{
	HANDLE hFile = CreateFileW(TextureAssetListPath, GENERIC_READ, 0, nullptr, OPEN_ALWAYS, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		SS_CLASS_ERR_LOG("file open failed. Error:%d", GetLastError());
		return E_FAIL;
	}

	kj::HandleInputStream isFile(hFile);
	kj::Array<byte> data = isFile.readAllBytes();

	capnp::MallocMessageBuilder message;
	SSTextureAssetManagingList::Builder textureList = message.initRoot<SSTextureAssetManagingList>();

	capnp::JsonCodec codec;
	codec.decode(data.asChars(), textureList);

	::capnp::List<SSTextureAssetManagingList::TextureAssetPair>::Reader texturePairList
		= textureList.getTextureList();

	const uint32 listSize = texturePairList.size();

	for(uint32 i=0;i<listSize;i++)
	{
		InsertNewAsset(DBG_NEW SSTextureAsset(
			texturePairList[i].getTextureName().cStr(),
			texturePairList[i].getTexturePath().cStr()));
		
	}

	// HACK: update on gpu buffer must be seperated
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

