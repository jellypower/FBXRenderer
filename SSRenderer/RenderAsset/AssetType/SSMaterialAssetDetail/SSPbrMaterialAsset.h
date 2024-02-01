#pragma once
#include "SSRenderer/RenderAsset/AssetType/SSMaterialAsset.h"

constexpr uint32 TX_BASE_COLOR_IDX = 0;
constexpr uint32 TX_METALLIC_IDX = 1;
constexpr uint32 TX_EMISSIVE_IDX = 2;
constexpr uint32 TX_NORMAL_IDX = 3;

struct alignas(16) PbrMaterialConstants
{
    Vector4f baseColorFactor;
    Vector4f emissiveFactor;
    float normalTextureScale;
    Vector2f metallicRoughnessFactor;
};

class SSPbrMaterialAsset : public SSMaterialAsset
{
    friend class SSFBXImporter;
private:
    PbrMaterialConstants _constantBuffer;

    
public:
    SSPbrMaterialAsset(const char* MaterialAssetName, SSShaderAsset* InShaderAsset);
    SSPbrMaterialAsset(const char* MaterialAssetName, const char* InShaderAssetName);

    FORCEINLINE void SetBaseColorFactor(Vector4f InColor) { _constantBuffer.baseColorFactor = InColor; }
    FORCEINLINE void SetEmissiveFactor(Vector4f InFactor) { _constantBuffer.emissiveFactor = InFactor; }
    FORCEINLINE void SetNormalTextureScale(float InScale) { _constantBuffer.normalTextureScale = InScale; }
    FORCEINLINE void SetMetallicRoughnessFactor(Vector2f InFactor) { _constantBuffer.metallicRoughnessFactor = InFactor; }

	virtual void InstantiateSystemBuffer() override;
	virtual void ReleaseSystemBuffer() override;

    virtual void InstantiateGPUBuffer(ID3D11Device* InDevice) override;
};

