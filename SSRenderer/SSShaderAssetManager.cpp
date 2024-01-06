#include "SSShaderAssetManager.h"
#include "ExternalUtils/ExternalUtils.h"
#include "SSEngineDefault/SSDebugLogger.h"

SSShaderAssetManager* SSShaderAssetManager::g_instance = nullptr;

SSShaderAssetManager::SSShaderAssetManager(uint32 poolCapacity)
{
	_shaderPoolCapacity = poolCapacity;
	ShaderList = DBG_NEW SSShaderAsset * [_shaderPoolCapacity];

}

SSShaderAssetManager::~SSShaderAssetManager()
{
	delete[] ShaderList;
}


void SSShaderAssetManager::LoadNewShaderTemp()
{
	ShaderList[_shaderPoolCount] = DBG_NEW SSShaderAsset(L"Resource/Shader/Tutorial07.fxh", "VS", "PS", "vs_4_0");
	_shaderPoolCount++;
}

HRESULT SSShaderAssetManager::CompileAllShader()
{
	
	for (uint32 i = 0; i < _shaderPoolCount; i++) {
		HRESULT hr = ShaderList[i]->CompileShader();
		if (FAILED(hr)) {
			SS_CLASS_ERR_LOG("Shader compile failed. ShaderIdx: %d", i);
			return hr;
		}
	}

	return S_OK;
}

HRESULT SSShaderAssetManager::InstantiateShader(uint32 ShaderName, ID3D11Device* InDevice)
{
	return ShaderList[ShaderName]->InstantiateShader(InDevice);
}

HRESULT SSShaderAssetManager::InstantiateAllShader(ID3D11Device* InDevice)
{
	for (uint32 i = 0; i < _shaderPoolCount; i++) {
		HRESULT hr = ShaderList[i]->InstantiateShader(InDevice);

		if (FAILED(hr)) {
			SS_CLASS_WARNING_LOG("Instantiate Failed.");
			return hr;
		}
	}
	return S_OK;
}

void SSShaderAssetManager::ReleaseShader(uint32 ShaderName)
{
	ShaderList[ShaderName]->Release();
	delete ShaderList[ShaderName];
	_shaderPoolCount--;
	for (uint32 i = ShaderName; i < _shaderPoolCount; i++) {
		ShaderList[i] = ShaderList[i + 1];
	}
}

void SSShaderAssetManager::ReleaseAllShader()
{
	for (uint32 i = 0; i < _shaderPoolCount; i++) {
		ShaderList[i]->Release();
		delete ShaderList[i];
	}


}

