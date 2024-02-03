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
};

class SSShaderAsset;
class SSTextureAsset;

enum class MaterialAssetInstanceStage {
	JustCreated = 0,
	SystemBufferInitialized = 1,
	GPUBufferInitialized,
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

	MaterialAssetInstanceStage _materialStage = MaterialAssetInstanceStage::JustCreated;

public:
	SSMaterialAsset(const char* MaterialAssetName, SSShaderAsset* InShaderAsset);
	SSMaterialAsset(const char* MaterialAssetName, const char* InShaderAssetName);

	virtual void InstantiateSystemBuffer();
	virtual void ReleaseSystemBuffer();

	virtual void InstantiateGPUBuffer(ID3D11Device* InDevice);
	virtual void ReleaseGPUBuffer();
	void Release();

	bool IsBindingPossible() const { return _materialStage >= MaterialAssetInstanceStage::GPUBufferInitialized; }
	void BindMaterial(ID3D11DeviceContext* InDeviceContext) const;

	void UpdateSystemBuffer(int bufferIdx, const void* InData, uint32 InDataSize);
	void SyncGPUBuffer(ID3D11DeviceContext* InDeviceContext, uint32 bufferIdx);
	void SyncAllGPUBuffer(ID3D11DeviceContext* InDeviceContext);

	void UpdateTransform(ID3D11DeviceContext* InDeviceContext, const Transform& InTransform);
	void UpdateTransform(ID3D11DeviceContext* InDeviceContext, const XMMATRIX& WMatrix, const XMMATRIX& RotMatrix);

	void UpdateGlobalRenderParam(ID3D11DeviceContext* InDeviceContext, const GlobalRenderParam& InParam);
	

	FORCEINLINE MaterialAssetInstanceStage GetMaterialStage() const { return _materialStage; }

};
