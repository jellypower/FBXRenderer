#include "SSSamplerPool.h"

#include "SSRenderer.h"

SSSamplerPool* SSSamplerPool::g_instance = nullptr;

D3D11_TEXTURE_ADDRESS_MODE ConvertAddressModeToNative(TexAddressMode InAddressMode)
{
	switch (InAddressMode)
	{
	case TexAddressMode::Repeat:
		return D3D11_TEXTURE_ADDRESS_WRAP;
	case TexAddressMode::MirroredRepeat:
		return D3D11_TEXTURE_ADDRESS_MIRROR;
	case TexAddressMode::ClampEdge:
		return D3D11_TEXTURE_ADDRESS_CLAMP;
	case TexAddressMode::UserDefinedBorder:
		return D3D11_TEXTURE_ADDRESS_BORDER;

	default:
		assert(false);
		return D3D11_TEXTURE_ADDRESS_WRAP;
	}
}

D3D11_FILTER ConvertFilterModeToNative(TexFilterMode filterMode)
{
	switch(filterMode)
	{
	case TexFilterMode::Linear:
		return D3D11_FILTER_MIN_MAG_MIP_LINEAR;

	default:
		assert(false);
		return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	}
}

SSSamplerPool::~SSSamplerPool()
{
	for (uint32 i = 0; i < (uint32)TexFilterMode::Count; i++)
	{
		for (uint32 j = 0; j < (uint32)TexAddressMode::Count; j++)
		{
			for (uint32 k = 0; k < (uint32)TexAddressMode::Count; k++)
			{
				if(_samplerCache[i][j][k] != nullptr)
					_samplerCache[i][j][k]->Release();
			}
		}
	}
}

ID3D11SamplerState* SSSamplerPool::GetSampler(TexFilterMode filterMode, TexAddressMode AddressU, TexAddressMode AddressV)
{
	if (_samplerCache[(uint32)filterMode][(uint32)AddressU][(uint32)AddressV] != nullptr)
		return _samplerCache[(uint32)filterMode][(uint32)AddressU][(uint32)AddressV];


	ID3D11Device* lDevice = nullptr;
	assert(_renderer != nullptr);
	if (_renderer->GetNativePlatformType() == NativePlatformType::WindowsD3D11)
		lDevice = static_cast<ID3D11Device*>(_renderer->GetNativeDevice());


	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = ConvertFilterModeToNative(filterMode);
	sampDesc.AddressU = ConvertAddressModeToNative(AddressU);
	sampDesc.AddressV = ConvertAddressModeToNative(AddressV);
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState* sampler;
	HRESULT hr = lDevice->CreateSamplerState(&sampDesc, &sampler);
	assert(SUCCEEDED(hr));

	_samplerCache[(uint32)filterMode][(uint32)AddressU][(uint32)AddressV] = sampler;

	return sampler;
}
