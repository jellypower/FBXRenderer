#pragma once
#include "SSRenderer/RenderAsset/AssetType/SSMaterialAsset.h"

enum TXTURE_IDX
{
    TX_BASE_COLOR_IDX = 0,
	TX_NORMAL_IDX = 1,
	TX_METALLIC_IDX = 2,
	TX_EMISSIVE_IDX = 3,
    TX_OCCLUSION_IDX = 4
};


struct alignas(16) PbrMaterialConstants
{
    Vector4f baseColorFactor;
    Vector4f emissiveFactor;
    float normalTextureScale;
    Vector2f metallicRoughnessFactor;
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
    FORCEINLINE void SetMetallicRoughnessFactor(Vector2f InFactor) { _constantBuffer.metallicRoughnessFactor = InFactor; }

	virtual void InstantiateSystemBuffer() override;
	virtual void ReleaseSystemBuffer() override;

    virtual void InstantiateGPUBuffer(ID3D11Device* InDevice) override;
};

