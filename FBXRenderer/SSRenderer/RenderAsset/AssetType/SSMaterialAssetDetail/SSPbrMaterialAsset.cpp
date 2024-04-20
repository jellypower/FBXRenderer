#include "SSPbrMaterialAsset.h"

#include "SSRenderer/RenderAsset/SSTextureAssetManager.h"
#include "SSRenderer/RenderAsset/SSShaderAssetManager.h"


SSPbrMaterialAsset::SSPbrMaterialAsset(const char* MaterialAssetName):
	SSMaterialAsset(MaterialAssetName, SSShaderAssetManager::SSDefaultPbrShaderName)
{
	_textureNames[SS_PBR_TX_BASE_COLOR_IDX] = SSTextureAssetManager::EmptyTextureName;
	_textureNames[SS_PBR_TX_NORMAL_IDX] = SSTextureAssetManager::EmptyNormalTextureName;
	_textureNames[SS_PBR_TX_METALLIC_IDX] = SSTextureAssetManager::WhiteTextureName;
	_textureNames[SS_PBR_TX_EMISSIVE_IDX] = SSTextureAssetManager::WhiteTextureName;
	_textureNames[SS_PBR_TX_OCCLUSION_IDX] = SSTextureAssetManager::WhiteTextureName;

	_constantBuffer.baseColorFactor = Vector4f::One;
	_constantBuffer.emissiveFactor = Vector4f::Zero;
	_constantBuffer.normalTextureScale = 1;
	_constantBuffer.metallicFactor = 0;
	_constantBuffer.roughnessFactor = 0;

	_explicitMaterialType = ExplicitMaterialType::SSDefaultPbrMaterial;
}

void SSPbrMaterialAsset::SetPBRTextureName(const char* NewTextureName, SS_PBR_TEXTURE_IDX TextureType)
{
	_textureNames[TextureType] = NewTextureName;
}

const char* SSPbrMaterialAsset::GetTextureName(SS_PBR_TEXTURE_IDX TextureType) const
{
	return _textureNames[TextureType];
}

void SSPbrMaterialAsset::SyncAllTextureItemWithName()
{
	for(uint32 i=0;i<SS_PBR_TX_COUNT;i++)
	{
		if (strcmp(_textureNames[i], _textureCache[i]->GetAssetName()) == 0) continue;

		SSTextureAsset* NewTexture = SSTextureAssetManager::FindAssetWithName(_textureNames[i], TexToReturnOnFail::Return_NULL);

		if(NewTexture != nullptr)
		{
			_textureCache[i] = NewTexture;
		}
	}
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

	_textureCache[SS_PBR_TX_BASE_COLOR_IDX] = SSTextureAssetManager::FindAssetWithName(_textureNames[SS_PBR_TX_BASE_COLOR_IDX]);
	_textureCache[SS_PBR_TX_NORMAL_IDX] = SSTextureAssetManager::FindAssetWithName(_textureNames[SS_PBR_TX_NORMAL_IDX], TexToReturnOnFail::EMPTYNORMAL);
	_textureCache[SS_PBR_TX_METALLIC_IDX] = SSTextureAssetManager::FindAssetWithName(_textureNames[SS_PBR_TX_METALLIC_IDX]);
	_textureCache[SS_PBR_TX_EMISSIVE_IDX] = SSTextureAssetManager::FindAssetWithName(_textureNames[SS_PBR_TX_EMISSIVE_IDX]);
	_textureCache[SS_PBR_TX_OCCLUSION_IDX] = SSTextureAssetManager::FindAssetWithName(_textureNames[SS_PBR_TX_OCCLUSION_IDX]);
}
