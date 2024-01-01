#pragma once
#include "SSEngineDefault/SSNativeTypes.h"


#include <windows.h>
#include <d3d11.h>



class SSTextureManager
{
public:
	HRESULT TempLoadTexture(ID3D11Device* InDevice);
	void Release();

	void ReleaseAllTextures();
	ID3D11ShaderResourceView* GetTextureByIndex(uint8 idx) { return TextureRVList[idx]; }
	
private:
	ID3D11ShaderResourceView** TextureRVList = nullptr;
	ID3D11Resource** TextureList = nullptr;
	uint8 TextureCount = 0;
};

