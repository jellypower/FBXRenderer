#pragma once
#include "SSEngineDefault/SSEngineDefault.h"


#include <windows.h>
#include <d3d11.h>

#include "AssetType/SSTextureAsset.h"
#include "SSEngineDefault/SSContainer/PooledList.h"
#include "SSEngineDefault/SSContainer/StringHashMapA.h"


enum class TexToReturnOnFail
{
	MAGENTA,
	WHITE,
	BLACK,
	EMPTYNORMAL,

	Return_NULL,

};

class SSTextureAssetManager
{
	friend class SSRenderer;
	SS_DECLARE_AS_SINGLETON(SSTextureAssetManager)
private:
	static FORCEINLINE void Instantiate(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed){
		if (g_instance != nullptr) {
			assert(false);
			return;
		}
		g_instance = DBG_NEW SSTextureAssetManager(poolCapacity, hashCapacity, hashCollisionLimit, hashSeed);
	}
public:
	static FORCEINLINE SSTextureAsset* FindAssetWithName(const char* name, TexToReturnOnFail OnFailReturn = TexToReturnOnFail::WHITE)
		{ return g_instance->FindAssetWithNameInternal(name, OnFailReturn); }
	static FORCEINLINE SSTextureAsset* GetAssetWithIdx(uint32 idx) { return g_instance->_textureList[idx]; }
	static FORCEINLINE uint32 GetAssetCount() { return g_instance->_textureList.GetSize(); }

	static constexpr char EmptyTextureName[] = "EMPTY";
	static constexpr char BlackTextureName[] = "BLACK";
	static constexpr char WhiteTextureName[] = "WHITE";
	static constexpr char EmptyNormalTextureName[] = "EMPTYNORMAL";

private:
	SS::PooledList<SSTextureAsset*> _textureList;
	SS::StringHashMapA<uint32, ASSET_NAME_LEN_MAX> _assetHashmap;

public:

	HRESULT LoadTextures(ID3D11Device* InDevice, const utf16* TextureAssetListPath);
	void ReleaseAllTextures();
	void InsertNewAsset(SSTextureAsset* newTextureAsset);

private:
	SSTextureAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed);
	~SSTextureAssetManager();

	SSTextureAsset* FindAssetWithNameInternal(const char* name, TexToReturnOnFail OnFailReturn);
	
};

