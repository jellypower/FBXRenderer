#include "SSPbrMaterialAsset.h"

#include "SSRenderer/RenderAsset/SSTextureAssetManager.h"


SSPbrMaterialAsset::SSPbrMaterialAsset(const char* MaterialAssetName, SSShaderAsset* InShaderAsset)
	: SSMaterialAsset(MaterialAssetName, InShaderAsset)
{ }

SSPbrMaterialAsset::SSPbrMaterialAsset(const char* MaterialAssetName, const char* InShaderAssetName)
	: SSMaterialAsset(MaterialAssetName, InShaderAssetName)
{ }

void SSPbrMaterialAsset::InstantiateSystemBuffer()
{
	SystemBufferData[MATERIAL_PARAM_START_IDX] = &_constantBuffer;
}

void SSPbrMaterialAsset::ReleaseSystemBuffer()
{
	SystemBufferData[MATERIAL_PARAM_START_IDX] = nullptr;
}

void SSPbrMaterialAsset::InstantiateGPUBuffer(ID3D11Device* InDevice)
{
	SSMaterialAsset::InstantiateGPUBuffer(InDevice);

	_textureCache[TX_BASE_COLOR_IDX] = SSTextureAssetManager::FindAssetWithName(_textureNames[TX_BASE_COLOR_IDX]);
	_textureCache[TX_METALLIC_IDX] = SSTextureAssetManager::FindAssetWithName(_textureNames[TX_METALLIC_IDX]);
	_textureCache[TX_EMISSIVE_IDX] = SSTextureAssetManager::FindAssetWithName(_textureNames[TX_EMISSIVE_IDX]);
	_textureCache[TX_NORMAL_IDX] = SSTextureAssetManager::FindAssetWithName(_textureNames[TX_NORMAL_IDX]);
}
