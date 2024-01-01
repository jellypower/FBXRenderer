#include "SSShaderAssetManager.h"
#include "ExternalUtils/ExternalUtils.h"
#include "SSEngineDefault/SSDebugLogger.h"


void SSShaderAssetManager::Init()
{
	// TODO: ���߿��� ����� ���̴� ����Ʈ�� "���Ϸκ��� �о" ������ �س��� Ǯ�� �÷�����
// �ϴ� ������ �׳� ���̴� Ǯ�� ���� ������ִ� �۾��� ����

	// TODO: ���߿� init���� HRESULT �־��ֱ�

	// TEMP
	ShaderList = DBG_NEW SSShaderAsset * [DEFAULT_POOL_SIZE];
	ShaderList[ShaderPoolCount] = DBG_NEW SSShaderAsset(L"Resource/Shader/Tutorial07.fxh", "VS", "PS", "vs_4_0");
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
			SS_CLASS_ERR_LOG("Shader compile failed. ShaderIdx: %d", i);
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

