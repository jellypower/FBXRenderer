#pragma once
#include <windows.h>
#include <d3d11.h>

#include "SSEngineDefault/SSNativeTypes.h"

#include "SSShaderAsset.h"
#include "SSEngineDefault/SSContainer/FixedList.h"
#include "SSEngineDefault/SSContainer/PooledList.h"


enum class MaterialAssetInstanceStage {
	JustCreated = 0,
	SystemBufferInitialized = 1,
	GPUBufferInitialized,
};

class SSMaterialAsset : public SSAssetBase
{
private:
	void* ConstBufferData[CONSTANT_BUFFER_COUNT_MAX] = { 0, };

	ID3D11Buffer* ConstantBuffers[CONSTANT_BUFFER_COUNT_MAX] = {};
	ID3D11Buffer* VSConstantBuffers[CONSTANT_BUFFER_COUNT_MAX] = {};
	ID3D11Buffer* PSConstantBuffers[CONSTANT_BUFFER_COUNT_MAX] = {};

	class SSShaderAsset* _shader = nullptr;
	const SSShaderReflectionForMaterial* _shaderReflectionCache;

	ID3D11ShaderResourceView* _textureList[TEXTURE_COUNT_MAX];
	ID3D11SamplerState* _sampleStateList[TEXTURE_COUNT_MAX];

	MaterialAssetInstanceStage _materialStage = MaterialAssetInstanceStage::JustCreated;

public:
	SSMaterialAsset(const char* MaterialAssetName, SSShaderAsset* InShaderAsset);
	SSMaterialAsset(const char* MaterialAssetName, const char* InShaderAssetName);

	void InstantiateCPUBuffer();
	void InstantiateGPUBuffer(ID3D11Device* InDevice);
	void Release();

	bool IsBindingPossible() { return _materialStage >= MaterialAssetInstanceStage::GPUBufferInitialized; }
	void BindMaterial(ID3D11DeviceContext* InDeviceContext) const;

	void UpdateSystemBuffer(int bufferIdx, const void* InData, uint32 InDataSize);
	void UpdateGPUBuffer(ID3D11DeviceContext* InDeviceContext, uint32 bufferIdx);
	void UpdateAllGPUBuffer(ID3D11DeviceContext* InDeviceContext, void** InConstBufferData, uint8 InConstBufferCount);

	void UpdateTransformGPUBuffer(ID3D11DeviceContext* InDeviceContext, const Transform& InTransform);
	void UpdateTransformGPUBuffer(ID3D11DeviceContext* InDeviceContext, const XMMATRIX& WMatrix, const XMMATRIX& RotMatrix);

	FORCEINLINE void UpdateCameraGPUBufferSetting(ID3D11DeviceContext* InDeviceContext, const XMMATRIX& InMatrix);
	

	FORCEINLINE MaterialAssetInstanceStage GetMaterialStage() const { return _materialStage; }

};


inline void SSMaterialAsset::UpdateCameraGPUBufferSetting(ID3D11DeviceContext* InDeviceContext, const XMMATRIX& InMatrix)
{
	InDeviceContext->UpdateSubresource(ConstantBuffers[VP_TRANSFORM_IDX], 0 ,nullptr, &InMatrix, 0 , 0);
}
