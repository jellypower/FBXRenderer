#include "SSShaderAssetManager.h"
#include "ExternalUtils/ExternalUtils.h"
#include "SSDebugLogger.h"


void SSShaderAssetManager::Init()
{
	// TODO: 나중에는 사용할 셰이더 리스트를 "파일로부터 읽어서" 컴파일 해놓고 풀에 올려놓기
// 일단 지금은 그냥 셰이더 풀을 직접 만들어주는 작업을 하자

	// TODO: 나중에 init에서 HRESULT 넣어주기

	// TEMP
	ShaderList = new SSShaderAsset * [DEFAULT_POOL_SIZE];
	ShaderList[ShaderPoolCount] = new SSShaderAsset(L"Resource/Shader/Tutorial07.fxh", "VS", "PS", "vs_4_0");
	ShaderPoolCount++;
	// TEMPEND
}

void SSShaderAssetManager::Release()
{
	delete[] ShaderList;
}

HRESULT SSShaderAssetManager::CompileAllShader()
{
	
	for (uint8 i = 0; i < ShaderPoolCount; i++) {
		HRESULT hr = ShaderList[i]->CompileShader();
		if (FAILED(hr)) {
			SS_LOG("Error(SSRenderer): Shader compile failed. ShaderIdx: %d\n", i);
			return hr;
		}
	}

	return S_OK;
}

HRESULT SSShaderAssetManager::InstantiateShader(uint8 ShaderName, ID3D11Device* InDevice)
{
	return ShaderList[ShaderName]->InstantiateShader(InDevice);
}

HRESULT SSShaderAssetManager::InstantiateAllShader(ID3D11Device* InDevice)
{
	for (uint8 i = 0; i < ShaderPoolCount; i++) {
		HRESULT hr = ShaderList[i]->InstantiateShader(InDevice);

		if (FAILED(hr)) {
			SS_CLASS_WARNING_LOG("Instantiate Failed.");
			return hr;
		}
	}
	return S_OK;
}

void SSShaderAssetManager::ReleaseShader(uint8 ShaderName)
{
	ShaderList[ShaderName]->Release();
	delete ShaderList[ShaderName];
	ShaderPoolCount--;
	for (uint8 i = ShaderName; i < ShaderPoolCount; i++) {
		ShaderList[i] = ShaderList[i + 1];
	}
}

void SSShaderAssetManager::ReleaseAllShader()
{
	for (uint8 i = 0; i < ShaderPoolCount; i++) {
		ShaderList[i]->Release();
		delete ShaderList[i];
	}


}

