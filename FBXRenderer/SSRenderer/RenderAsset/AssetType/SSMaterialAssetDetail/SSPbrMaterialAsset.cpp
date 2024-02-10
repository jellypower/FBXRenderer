#include "SSPbrMaterialAsset.h"

#include "SSRenderer/RenderAsset/SSTextureAssetManager.h"
#include "SSRenderer/RenderAsset/SSShaderAssetManager.h"


SSPbrMaterialAsset::SSPbrMaterialAsset(const char* MaterialAssetName)
	: SSMaterialAsset(MaterialAssetName, SSShaderAssetManager::SSDefaultPbrShaderName)
{
	_textureNames[TX_BASE_COLOR_IDX] = SSTextureAssetManager::EmptyTextureName;
	_textureNames[TX_NORMAL_IDX] = SSTextureAssetManager::EmptyNormalTextureName;
	_textureNames[TX_METALLIC_IDX] = SSTextureAssetManager::WhiteTextureName;
	_textureNames[TX_EMISSIVE_IDX] = SSTextureAssetManager::WhiteTextureName;
	_textureNames[TX_OCCLUSION_IDX] = SSTextureAssetManager::WhiteTextureName;

	_constantBuffer.baseColorFactor = Vector4f::One;
	_constantBuffer.emissiveFactor = Vector4f::Zero;
	_constantBuffer.normalTextureScale = 1;
	_constantBuffer.metallicRoughnessFactor = Vector2f(0, 0);
}

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
	_textureCache[TX_NORMAL_IDX] = SSTextureAssetManager::FindAssetWithName(_textureNames[TX_NORMAL_IDX]);
	_textureCache[TX_METALLIC_IDX] = SSTextureAssetManager::FindAssetWithName(_textureNames[TX_METALLIC_IDX]);
	_textureCache[TX_EMISSIVE_IDX] = SSTextureAssetManager::FindAssetWithName(_textureNames[TX_EMISSIVE_IDX]);
	_textureCache[TX_OCCLUSION_IDX] = SSTextureAssetManager::FindAssetWithName(_textureNames[TX_OCCLUSION_IDX]);
}
