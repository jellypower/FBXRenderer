#pragma once
#include <d3d11.h>
#include <windows.h>

#include "SSEngineDefault/SSNativeTypes.h"

#include "SSEngineDefault/SSContainer/FixedList.h"
#include "SSEngineDefault/SSContainer/PooledList.h"
#include "SSShaderAsset.h"


struct alignas(16) GlobalRenderParam
{
	XMMATRIX VPMatrix;
	Vector4f SunDirection;
	Vector4f SunIntensity;
	Vector4f ViewerPos;
};

class SSShaderAsset;
class SSTextureAsset;

enum class MaterialAssetInstanceStage {
	JustCreated = 0,
	SystemBufferInitialized = 1,
	GPUBufferInitialized,
};

static const char* ExplicitMaterialTypeStr[]
{
	"None",
	"SSDefaultPbrMaterial",
	
	"Custom",
	"Count"
};

enum class ExplicitMaterialType
{
	None,
	SSDefaultPbrMaterial,

	Custom,
	Count
};

class SSMaterialAsset : public SSAssetBase
{
	friend class SSMaterialAssetManager;
	friend class SSFBXImporter;
protected:
	void* SystemBufferData[CONSTANT_BUFFER_COUNT_MAX] = { 0, };

	ID3D11Buffer* ConstantBuffers[CONSTANT_BUFFER_COUNT_MAX] = {};
	ID3D11Buffer* VSConstantBuffers[CONSTANT_BUFFER_COUNT_MAX] = {};
	ID3D11Buffer* PSConstantBuffers[CONSTANT_BUFFER_COUNT_MAX] = {};

	SSShaderAsset* _shader = nullptr;
	const SSShaderReflectionForMaterial* _shaderReflectionCache;

	SS::FixedStringA<ASSET_NAME_LEN_MAX> _textureNames[TEXTURE_COUNT_MAX];
	SSTextureAsset* _textureCache[TEXTURE_COUNT_MAX];
	ID3D11SamplerState* _sampleCache[TEXTURE_COUNT_MAX];

	ExplicitMaterialType _explicitMaterialType = ExplicitMaterialType::None;
	MaterialAssetInstanceStage _materialStage = MaterialAssetInstanceStage::JustCreated;

	// HACK: 
	struct PbrMaterialParamEditorCopy* _tempParams;

public:
	SSMaterialAsset(const char* MaterialAssetName, SSShaderAsset* InShaderAsset);
	SSMaterialAsset(const char* MaterialAssetName, const char* InShaderAssetName);

	virtual void InstantiateSystemBuffer();
	virtual void ReleaseSystemBuffer();

	virtual void InstantiateGPUBuffer(ID3D11Device* InDevice);
	virtual void ReleaseGPUBuffer();
	void Release();

	bool IsBindingPossible() const { return _materialStage >= MaterialAssetInstanceStage::GPUBufferInitialized; }

	void ChangeShader(const char* InShaderAssetName);
	void ChangeShader(SSShaderAsset* InShaderAsset);


	void UpdateSystemBuffer(int bufferIdx, const void* InData, uint32 InDataSize);
	void SyncGPUBuffer(ID3D11DeviceContext* InDeviceContext, uint32 bufferIdx);
	void SyncAllGPUBuffer(ID3D11DeviceContext* InDeviceContext);

	void UpdateTransform(ID3D11DeviceContext* InDeviceContext, const Transform& InTransform);
	void UpdateTransform(ID3D11DeviceContext* InDeviceContext, const XMMATRIX& WMatrix, const XMMATRIX& RotMatrix);

	void UpdateGlobalRenderParam(ID3D11DeviceContext* InDeviceContext, const GlobalRenderParam& InParam);
	
	FORCEINLINE ExplicitMaterialType GetExplicitMaterialType() const { return _explicitMaterialType; }
	FORCEINLINE MaterialAssetInstanceStage GetMaterialStage() const { return _materialStage; }


	FORCEINLINE SSShaderAsset* GetShader() const { return _shader; }
	FORCEINLINE uint8 GetVSConstantBufferCnt() const { return _shaderReflectionCache->VSConstBufferNum; }
	FORCEINLINE uint8 GetPSConstantBufferCnt() const { return _shaderReflectionCache->PSConstBufferNum; }
	
	FORCEINLINE uint8 GetVSConstantBufferIdx(uint8 constantBufferIdx) const { return _shaderReflectionCache->VSCBReflectionInfo[constantBufferIdx].CBSlotIdx; }
	FORCEINLINE uint8 GetPSConstantBufferIdx(uint8 constantBufferIdx) const { return _shaderReflectionCache->PSCBReflectionInfo[constantBufferIdx].CBSlotIdx; }
	FORCEINLINE ID3D11Buffer* const* GetVSConstantBufferPtr(uint8 constantBufferIdx) const { return &VSConstantBuffers[constantBufferIdx]; }
	FORCEINLINE ID3D11Buffer* const* GetPSConstantBufferPtr(uint8 constantBufferIdx) const { return &PSConstantBuffers[constantBufferIdx]; }

	FORCEINLINE uint8 GetTextureCnt() const { return _shaderReflectionCache->TexturePoolCount; }
	FORCEINLINE SSTextureAsset* GetBoundTextureAsset(uint8 textureIdx) const { return _textureCache[textureIdx]; }
	FORCEINLINE uint8 GetSamplerCnt() const { return _shaderReflectionCache->SamplerCount; }
	FORCEINLINE ID3D11SamplerState* const* GetSamplerPtr() const { return _sampleCache; }

};
