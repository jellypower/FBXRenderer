#include "SSShaderAssetManager.h"
#include "ExternalUtils/ExternalUtils.h"
#include "SSEngineDefault/SSDebugLogger.h"

SSShaderAssetManager* SSShaderAssetManager::g_instance = nullptr;

SSShaderAssetManager::SSShaderAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed)
	: _assetPool(poolCapacity), _assetHashMap(hashCapacity, hashCollisionLimit, hashSeed)
{
}

SSShaderAssetManager::~SSShaderAssetManager()
{

}

SSShaderAsset* SSShaderAssetManager::FindShaderWithNameInternal(const char* name)
{
	uint32 outResult = SS_UINT32_MAX;

	if (_assetHashMap.TryFind(name, outResult) == FindResult::Success)
		return _assetPool[outResult];

	return nullptr;
}


void SSShaderAssetManager::InsertNewShaderAsset(SSShaderAsset* newShader)
{
	_assetPool.PushBack(newShader);
	InsertResult result =  _assetHashMap.TryInsert(newShader->GetAssetName(), _assetPool.GetSize() - 1);

	switch(result)
	{
	case InsertResult::KeyAlreadyExist:
		WASSERT_WITH_MESSAGE(false, "Shader asset's name must be unique");
		break;
	case InsertResult::Success:
		break;
	default:
		WASSERT_WITH_MESSAGE(false, "Error!");
		break;
	}

}

void SSShaderAssetManager::LoadNewShaderTemp()
{
	InsertNewShaderAsset(DBG_NEW SSShaderAsset("SSDefaultPbr", L"Resource/Shader/Phong.fxh", "VS", "PS", "vs_4_0"));
}

HRESULT SSShaderAssetManager::CompileAllShader()
{
	for(SSShaderAsset* asset : _assetPool )
	{
		HRESULT hr = asset->CompileShader();
		if (FAILED(hr)) {
			SS_CLASS_ERR_LOG("Shader compile failed. ShaderName: %ls", asset->GetAssetPath());
			return hr;
		}
	}

	return S_OK;
}

HRESULT SSShaderAssetManager::InstantiateShaderWitIdx(uint32 ShaderName, ID3D11Device* InDevice)
{
	return _assetPool[ShaderName]->InstantiateShader(InDevice);
}

HRESULT SSShaderAssetManager::InstantiateAllShader(ID3D11Device* InDevice)
{
	for (SSShaderAsset* item : _assetPool) {
		HRESULT hr = item->InstantiateShader(InDevice);

		if (FAILED(hr)) {
			SS_CLASS_WARNING_LOG("Instantiate Failed. Shader: %ls", item->GetAssetPath());
			return hr;
		}
	}
	return S_OK;
}

void SSShaderAssetManager::ReleaseShaderWithIdx(uint32 idx)
{
	_assetPool[idx]->Release();
	delete _assetPool[idx];
	_assetPool[idx] = nullptr;
}

void SSShaderAssetManager::ReleaseAllShader()
{
	for(SSShaderAsset* item : _assetPool)
	{
		item->Release();
		delete item;
	}

}

