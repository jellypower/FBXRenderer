#include "SSTextureManager.h"

#include "SSEngineDefault/SSDebugLogger.h"
#include "ExternalUtils/DDSTextureLoader.h"

using namespace DirectX;

SSTextureManager* SSTextureManager::g_instance = nullptr;

SSTextureManager::SSTextureManager(uint32 poolCount)
{
	_texturePoolCapacity = poolCount;
	TextureRVList = DBG_NEW ID3D11ShaderResourceView * [_texturePoolCapacity];
	TextureList = DBG_NEW ID3D11Resource * [_texturePoolCapacity];
}

SSTextureManager::~SSTextureManager()
{
	delete[] TextureRVList;
	delete[] TextureList;
}


HRESULT SSTextureManager::TempLoadTexture(ID3D11Device* InDevice)
{
	_texturePoolCount = 1;

	HRESULT hr = S_OK;
	hr = CreateDDSTextureFromFile(InDevice, L"Resource/Texture/seafloor.dds", &TextureList[0], &TextureRVList[0]);
	if (FAILED(hr)) {
		SS_LOG("Error(SSTextureManager::TempLoadTexture): texture load failed.\n");
		return hr;
	}

	return hr;
}


void SSTextureManager::ReleaseAllTextures()
{
	for (int i = 0; i < _texturePoolCount; i++) {
		TextureRVList[i]->Release();
		TextureList[i]->Release();
	}

}

