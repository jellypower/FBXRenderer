#pragma once
#include "SSEngineDefault/SSEngineDefault.h"


#include <windows.h>
#include <d3d11.h>



class SSTextureManager
{
	friend class SSRenderer;
	SS_DECLARE_AS_SINGLETON(SSTextureManager)
private:
	static FORCEINLINE void Instantiate(uint32 poolCapacity){
		if (g_instance != nullptr) {
			assert(false);
			return;
		}
		g_instance = DBG_NEW SSTextureManager(poolCapacity);
	}

private:
	ID3D11ShaderResourceView** TextureRVList = nullptr;
	ID3D11Resource** TextureList = nullptr;
	uint32 _texturePoolCount = 0;
	uint32 _texturePoolCapacity = 0;

public:
	HRESULT TempLoadTexture(ID3D11Device* InDevice);
	void ReleaseAllTextures();
	ID3D11ShaderResourceView* GetTextureByIndex(uint32 idx) { return TextureRVList[idx]; }
	
private:
	SSTextureManager(uint32 poolCount);
	~SSTextureManager();


};

