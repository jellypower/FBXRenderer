#include "SSMaterial.h"

#include "SSDebug.h"
#include "SSTextureManager.h"

#include <directxmath.h>

using namespace DirectX;

HRESULT SSMaterial::InitTemp(ID3D11Device* InDevice,
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

void SSMaterial::Release()
{
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



void SSMaterial::BindMaterial(ID3D11DeviceContext* InDeviceContext)
{
	if (MaterialStage != MaterialAssetInstanceStage::Initialized) {
		SS_LOG("Error(SSMaterial): To bind material, Material must be initialized\n");
		return;
	}

	Shader->UpdateAllShaderCBData(InDeviceContext, ConstBufferData, ShaderReflectionMetadata->EntireConstBufferNum);
		
	Shader->BindShaderAsset(InDeviceContext);

	InDeviceContext->PSSetShaderResources(0, ShaderReflectionMetadata->TextureCount, TextureList);
	InDeviceContext->PSSetSamplers(0, ShaderReflectionMetadata->SamplerCount, SampleStateList);

}


void SSMaterial::UpdateParameter(int idx, const void* InData, uint32 InDataSize)
{
#ifdef _DEBUG
	if (InDataSize != ShaderReflectionMetadata->EntireCBReflectionInfo[idx].CBSize) {
		SS_CLASS_ERR_LOG("Data size not match\n");
		return;
	}
#endif
	memcpy_s(ConstBufferData[idx], ShaderReflectionMetadata->EntireCBReflectionInfo[idx].CBSize, InData, InDataSize);
}
