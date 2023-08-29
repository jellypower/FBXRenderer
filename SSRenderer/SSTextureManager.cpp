#include "SSTextureManager.h"

#include "SSDebug.h"
#include "ExternalUtils/DDSTextureLoader.h"

using namespace DirectX;

HRESULT SSTextureManager::TempLoadTexture(ID3D11Device* InDevice)
{
	TextureCount = 1;
	TextureRVList = new ID3D11ShaderResourceView*[TextureCount];
	TextureList = new ID3D11Resource*[TextureCount];

	HRESULT hr = S_OK;

	hr = CreateDDSTextureFromFile(InDevice, L"Resource/Texture/seafloor.dds", &TextureList[0], &TextureRVList[0]);
	if (FAILED(hr)) {
		SS_LOG("Error(SSTextureManager::TempLoadTexture): texture load failed.\n");
		return hr;
	}



	return hr;
}

void SSTextureManager::Release()
{
	delete[] TextureRVList;
	delete[] TextureList;
}

void SSTextureManager::ReleaseAllTextures()
{
	for (int i = 0; i < TextureCount; i++) {
		TextureRVList[i]->Release();
		TextureList[i]->Release();
	}

}
