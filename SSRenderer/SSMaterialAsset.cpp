#include "SSMaterialAsset.h"

#include "SSDebugLogger.h"
#include "SSTextureManager.h"

#include <directxmath.h>

using namespace DirectX;

HRESULT SSMaterialAsset::InitTemp(ID3D11Device* InDevice,
	SSShaderAsset* InShaderAsset, SSTextureManager* InTextureManager)
{
	if (InShaderAsset->GetShaderInstanceStage() < ShaderAssetInstanceStage::Instantiated) {
		SS_LOG("Error(SSMaterial): To make material, shader asset layout must be initialized \n");
		return E_FAIL;
	}

	Shader = InShaderAsset;
	ShaderReflectionMetadata = Shader->GetShaderReflectionPointer();

	
	TextureList = new ID3D11ShaderResourceView * [ShaderReflectionMetadata->TextureCount];
	SampleStateList = new ID3D11SamplerState * [ShaderReflectionMetadata->SamplerCount];



	
	for (int i = 0; i < ShaderReflectionMetadata->EntireConstBufferNum; i++) {
		ConstBufferData[i] = malloc(ShaderReflectionMetadata->EntireCBReflectionInfo[i].CBSize);
		ZeroMemory(ConstBufferData[i], ShaderReflectionMetadata->EntireCBReflectionInfo[i].CBSize);

	}



	// HACK: start
	TextureList[0] = InTextureManager->GetTextureByIndex(0);

	D3D11_SAMPLER_DESC sampleDesc = {};
	sampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampleDesc.MinLOD = 0;
	sampleDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HRESULT hr = InDevice->CreateSamplerState(&sampleDesc, &SampleStateList[0]);

	if (FAILED(hr)) {
		SS_LOG("Error(SSMaterial::InitTemp): Sample State creation failed.");
		return hr;
	}

	// HACK: end



	MaterialStage = MaterialAssetInstanceStage::Initialized;
	return S_OK;

}

void SSMaterialAsset::Release()
{

	for (int i = 0; i < ShaderReflectionMetadata->EntireConstBufferNum; i++) {
		ConstantBuffers[i]->Release();
	}

	for (int i = 0; i < ShaderReflectionMetadata->EntireConstBufferNum; i++) {
		free(ConstBufferData[i]);
	}

	for (int i = 0; i < ShaderReflectionMetadata->TextureCount; i++) {
		SampleStateList[i]->Release();
	}

	delete[] SampleStateList;
	delete[] TextureList;

	ShaderReflectionMetadata = {};
	MaterialStage = MaterialAssetInstanceStage::JustCreated;
	Shader = nullptr;
}



void SSMaterialAsset::BindMaterial(ID3D11DeviceContext* InDeviceContext)
{
	if (MaterialStage <  MaterialAssetInstanceStage::Initialized) {
		SS_LOG("Error(SSMaterial): To bind material, Material must be initialized\n");
		return;
	}

	Shader->BindShaderAsset(InDeviceContext);


	for (int i = 0; i < ShaderReflectionMetadata->VSConstBufferNum; i++) {
		InDeviceContext->VSSetConstantBuffers(ShaderReflectionMetadata->VSCBReflectionInfo[i].CBSlotIdx, 1,
			&VSConstantBuffers[i]);
	}
	for (int i = 0; i < ShaderReflectionMetadata->PSConstBufferNum; i++) {
		InDeviceContext->PSSetConstantBuffers(ShaderReflectionMetadata->PSCBReflectionInfo[i].CBSlotIdx, 1,
			&PSConstantBuffers[i]);
	}


	InDeviceContext->PSSetShaderResources(0, ShaderReflectionMetadata->TextureCount, TextureList);
	InDeviceContext->PSSetSamplers(0, ShaderReflectionMetadata->SamplerCount, SampleStateList);

}


void SSMaterialAsset::UpdateParameter(ID3D11DeviceContext* InDeviceContext ,int idx, const void* InData, uint32 InDataSize)
{
#ifdef _DEBUG
	if (InDataSize != ShaderReflectionMetadata->EntireCBReflectionInfo[idx].CBSize) {
		SS_CLASS_ERR_LOG("Data size not match\n");
		return;
	}
#endif
	memcpy_s(ConstBufferData[idx], ShaderReflectionMetadata->EntireCBReflectionInfo[idx].CBSize, InData, InDataSize);
	InDeviceContext->UpdateSubresource(ConstantBuffers[idx], 0, nullptr, InData, 0,0);
	
}

HRESULT SSMaterialAsset::InstantiateShader(ID3D11Device* InDevice)
{

	HRESULT hr = S_OK;

	uint8 CurVSCBIdx = 0;
	uint8 CurPSCBIdx = 0;

	for (uint8 i = 0; i < ShaderReflectionMetadata->EntireConstBufferNum; i++) {
		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DEFAULT; // TODO: 나중에 dynamic으로 바꿔보기
		bd.ByteWidth = ShaderReflectionMetadata->EntireCBReflectionInfo[i].CBSize;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;


		hr = InDevice->CreateBuffer(&bd, nullptr, &ConstantBuffers[i]);



		if (ShaderReflectionMetadata->EntireCBReflectionInfo[i].bCBUsedByVSShader) {
			VSConstantBuffers[CurVSCBIdx++] = ConstantBuffers[i];
		}

		if (ShaderReflectionMetadata->EntireCBReflectionInfo[i].bCBUsedByPSShader) {
			PSConstantBuffers[CurPSCBIdx++] = ConstantBuffers[i];
		}

		if (FAILED(hr)) {
			SS_CLASS_ERR_LOG("Material creation failed");
			return hr;
		}
	}

	MaterialStage = MaterialAssetInstanceStage::Instantiated;

	return hr;
}

void SSMaterialAsset::UpdateAllShaderCBData(ID3D11DeviceContext* InDeviceContext, void** InConstBufferData, uint8 InConstBufferCount)
{
	for (int i = WVP_TRANSFOMRM_IDX + 1; i < InConstBufferCount; i++) {
		InDeviceContext->UpdateSubresource(ConstantBuffers[i], 0, nullptr, InConstBufferData[i], 0, 0);
	}
}