#pragma once
#include "SSRenderer/RenderAsset/AssetType/SSMaterialAsset.h"

enum SS_PBR_TEXTURE_IDX
{
    SS_PBR_TX_BASE_COLOR_IDX = 0,
	SS_PBR_TX_NORMAL_IDX = 1,
	SS_PBR_TX_METALLIC_IDX = 2,
	SS_PBR_TX_EMISSIVE_IDX = 3,
    SS_PBR_TX_OCCLUSION_IDX = 4,

    SS_PBR_TX_COUNT = 5
};


struct alignas(16) PbrMaterialConstants
{
    Vector4f baseColorFactor;
    Vector4f emissiveFactor;
    float normalTextureScale;
    float metallicFactor;
	float roughnessFactor;
};


constexpr uint32 tempSize = sizeof(PbrMaterialConstants);

class SSPbrMaterialAsset : public SSMaterialAsset
{
    friend class SSFBXImporter;
private:
    PbrMaterialConstants _constantBuffer;

    
public:
    SSPbrMaterialAsset(const char* MaterialAssetName);

    FORCEINLINE void SetBaseColorFactor(Vector4f InColor) { _constantBuffer.baseColorFactor = InColor; }
    FORCEINLINE void SetEmissiveFactor(Vector4f InFactor) { _constantBuffer.emissiveFactor = InFactor; }
    FORCEINLINE void SetNormalTextureScale(float InScale) { _constantBuffer.normalTextureScale = InScale; }
    FORCEINLINE void SetMetallicFactor(float InFactor) { _constantBuffer.metallicFactor = InFactor; }
    FORCEINLINE void SetRoughnessFactor(float InFactor) { _constantBuffer.roughnessFactor = InFactor; }

    FORCEINLINE PbrMaterialConstants GetMaterialParam() const { return _constantBuffer; }
    FORCEINLINE void SetMaterialParam(const PbrMaterialConstants& InParam) { _constantBuffer = InParam; }


    void SetPBRTextureName(const char* NewTextureName, SS_PBR_TEXTURE_IDX TextureType);
    const char* GetTextureName(SS_PBR_TEXTURE_IDX TextureType) const;
    void SyncAllTextureItemWithName();
    


	virtual void InstantiateSystemBuffer() override;
	virtual void ReleaseSystemBuffer() override;

    virtual void InstantiateGPUBuffer(ID3D11Device* InDevice) override;
};

