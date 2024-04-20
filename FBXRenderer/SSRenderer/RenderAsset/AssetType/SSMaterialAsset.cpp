#include "SSMaterialAsset.h"

#include "SSEngineDefault/SSDebugLogger.h"


#include <directxmath.h>

#include "../SSShaderAssetManager.h"
#include "SSTextureAsset.h"
using namespace DirectX;

SSMaterialAsset::SSMaterialAsset(const char* MaterialAssetName, SSShaderAsset* InShaderAsset)
	: SSAssetBase(AssetType::Material)
{
	if (InShaderAsset->GetShaderInstanceStage() < ShaderAssetInstanceStage::Instantiated) {
		SS_CLASS_ERR_LOG("To make material, shader asset layout must be initialized \n");
		return;
	}

	_assetName = MaterialAssetName;
	_shader = InShaderAsset;
	_shaderReflectionCache = _shader->GetShaderReflectionPtr();

	_materialStage = MaterialAssetInstanceStage::JustCreated;
}

SSMaterialAsset::SSMaterialAsset(const char* MaterialAssetName, const char* InShaderAssetName)
	: SSAssetBase(AssetType::Material)
{
	SSShaderAsset* shaderAsset = SSShaderAssetManager::FindShaderWithName(InShaderAssetName);
	if (InShaderAssetName == nullptr)
	{
		SS_CLASS_ERR_LOG("This name of material not found. (shaderName: %s)\n", InShaderAssetName);
		return;
	}
	if (shaderAsset->GetShaderInstanceStage() < ShaderAssetInstanceStage::Instantiated)
	{
		SS_CLASS_ERR_LOG("To make material, shader asset layout must be initialized. (shaderName: %s)\n", InShaderAssetName);
		return;
	}

	_assetName = MaterialAssetName;
	_shader = shaderAsset;
	_shaderReflectionCache = _shader->GetShaderReflectionPtr();

	_materialStage = MaterialAssetInstanceStage::JustCreated;
}

void SSMaterialAsset::InstantiateSystemBuffer()
{
	for (int i = MATERIAL_PARAM_START_IDX; i < _shaderReflectionCache->EntireConstBufferNum; i++) {
		SystemBufferData[i] = malloc(_shaderReflectionCache->EntireCBReflectionInfo[i].CBSize);
		ZeroMemory(SystemBufferData[i], _shaderReflectionCache->EntireCBReflectionInfo[i].CBSize);
	}

	_materialStage = MaterialAssetInstanceStage::SystemBufferInitialized;
}

void SSMaterialAsset::ReleaseSystemBuffer()
{
	for (int i = MATERIAL_PARAM_START_IDX; i < _shaderReflectionCache->EntireConstBufferNum; i++) {
		free(SystemBufferData[i]);
	}
}

void SSMaterialAsset::Release()
{
	ReleaseGPUBuffer();
	ReleaseSystemBuffer();

	_materialStage = MaterialAssetInstanceStage::JustCreated;
}



void SSMaterialAsset::InstantiateGPUBuffer(ID3D11Device* InDevice)
{
	uint8 CurVSCBIdx = 0;
	uint8 CurPSCBIdx = 0;

	for (uint8 i = 0; i < _shaderReflectionCache->ConstBufferSlotMax + 1; i++) {

		if (_shaderReflectionCache->EntireCBReflectionInfo[i].CBSize == INVALID_BUFFER_SIZE) continue;

		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DEFAULT; // TODO: 나중에 dynamic으로 바꿔보기
		bd.ByteWidth = _shaderReflectionCache->EntireCBReflectionInfo[i].CBSize;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;

		InDevice->CreateBuffer(&bd, nullptr, &ConstantBuffers[i]);


		if (_shaderReflectionCache->EntireCBReflectionInfo[i].bCBUsedByVSShader) {
			VSConstantBuffers[CurVSCBIdx++] = ConstantBuffers[i];
		}

		if (_shaderReflectionCache->EntireCBReflectionInfo[i].bCBUsedByPSShader) {
			PSConstantBuffers[CurPSCBIdx++] = ConstantBuffers[i];
		}
	}

	_materialStage = MaterialAssetInstanceStage::GPUBufferInitialized;
}

void SSMaterialAsset::ReleaseGPUBuffer()
{
	for (int i = 0; i < _shaderReflectionCache->EntireConstBufferNum; i++) {
		if (ConstantBuffers[i] != nullptr)
			ConstantBuffers[i]->Release();
		ConstantBuffers[i] = nullptr;
	}
}

void SSMaterialAsset::ChangeShader(const char* InShaderAssetName)
{
	SSShaderAsset* shaderAsset = SSShaderAssetManager::FindShaderWithName(InShaderAssetName);
	if (InShaderAssetName == nullptr)
	{
		SS_CLASS_ERR_LOG("This name of material not found. (shaderName: %s)\n", InShaderAssetName);
		return;
	}
	if (shaderAsset->GetShaderInstanceStage() < ShaderAssetInstanceStage::Instantiated)
	{
		SS_CLASS_ERR_LOG("To make material, shader asset layout must be initialized. (shaderName: %s)\n", InShaderAssetName);
		return;
	}

	_shader = shaderAsset;
	_shaderReflectionCache = _shader->GetShaderReflectionPtr();
}

void SSMaterialAsset::ChangeShader(SSShaderAsset* InShaderAsset)
{
	if (InShaderAsset->GetShaderInstanceStage() < ShaderAssetInstanceStage::Instantiated) {
		SS_CLASS_ERR_LOG("To make material, shader asset layout must be initialized \n");
		return;
	}
	_shader = InShaderAsset;
	_shaderReflectionCache = _shader->GetShaderReflectionPtr();
}

void SSMaterialAsset::UpdateSystemBuffer(int bufferIdx, const void* InData, uint32 InDataSize)
{
	SS_ASSERT(InDataSize == _shaderReflectionCache->EntireCBReflectionInfo[bufferIdx].CBSize, "Data size not match\n");
	memcpy_s(SystemBufferData[bufferIdx], _shaderReflectionCache->EntireCBReflectionInfo[bufferIdx].CBSize, InData, InDataSize);
}

void SSMaterialAsset::SyncGPUBuffer(ID3D11DeviceContext* InDeviceContext, uint32 bufferIdx)
{
	InDeviceContext->UpdateSubresource(ConstantBuffers[bufferIdx], 0, nullptr, SystemBufferData[bufferIdx], 0, 0);
}



void SSMaterialAsset::SyncAllGPUBuffer(ID3D11DeviceContext* InDeviceContext)
{
	for (uint32 i = MATERIAL_PARAM_START_IDX; i < _shaderReflectionCache->EntireConstBufferNum; i++) {
		if(SystemBufferData[i] != nullptr)
			InDeviceContext->UpdateSubresource(ConstantBuffers[i], 0, nullptr, SystemBufferData[i], 0, 0);
	}
}


void SSMaterialAsset::UpdateTransform(ID3D11DeviceContext* InDeviceContext, const Transform& InTransform)
{
	XMMATRIX worldMatrix[2];
	worldMatrix[0] = XMMatrixTranspose(InTransform.AsMatrix());
	worldMatrix[1] = XMMatrixTranspose(InTransform.Rotation.AsMatrix());
	InDeviceContext->UpdateSubresource(ConstantBuffers[W_TRANSFOMRM_IDX], 0, nullptr, worldMatrix, 0, 0);
}

void SSMaterialAsset::UpdateTransform(ID3D11DeviceContext* InDeviceContext, const XMMATRIX& WMatrix, const XMMATRIX& RotMatrix)
{
	XMMATRIX worldMatrix[2];
	worldMatrix[0] = XMMatrixTranspose(WMatrix);
	worldMatrix[1] = XMMatrixTranspose(RotMatrix);
	InDeviceContext->UpdateSubresource(ConstantBuffers[W_TRANSFOMRM_IDX], 0, nullptr, worldMatrix, 0, 0);
}

void SSMaterialAsset::UpdateGlobalRenderParam(ID3D11DeviceContext* InDeviceContext, const GlobalRenderParam& InParam)
{
	InDeviceContext->UpdateSubresource(ConstantBuffers[GLOBAL_RENDER_PARAM_IDX], 0, nullptr, &InParam, 0, 0);
}
