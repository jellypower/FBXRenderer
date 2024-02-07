#include "SSShaderAsset.h"



#include <directxmath.h>
#include <wrl.h>


#include "ExternalUtils/ExternalUtils.h"

#include "SSEngineDefault/SSDebugLogger.h"


using namespace DirectX;
namespace WRL = Microsoft::WRL; // Windows Runtime Library

SSShaderAsset::SSShaderAsset(const char* InShaderName, const utf16* InShaderPath, LPCSTR szVSEntryPoint, LPCSTR szPSEntryPoint, LPCSTR szShaderModel)
	: SSAssetBase(AssetType::Shader)
{

	if (strlen(InShaderName) > ASSET_NAME_LEN_MAX) {
		SS_LOG("Error(SSShaderAsset::SSShaderAsset): Shader file name too long\n");
		return;
	}

	if (strlen(szVSEntryPoint) > ASSET_NAME_LEN_MAX) {
		SS_LOG("Error(SSShaderAsset::SSShaderAsset): VS Shader entry point name too long\n");
		return;
	}

	if (strlen(szPSEntryPoint) > ASSET_NAME_LEN_MAX) {
		SS_LOG("Error(SSShaderAsset::SSShaderAsset): PS Shader entry point name too long\n");
		return;
	}

	_assetPath = InShaderPath;
	_assetName = InShaderName;
	_vsShaderEntryPoint = szVSEntryPoint;
	_psShaderEntryPoint = szPSEntryPoint;

	_curStage = ShaderAssetInstanceStage::JustCreated;
}

HRESULT SSShaderAsset::CompileShader()
{

	HRESULT hr =
		ExternalUtils::CompileShaderFromFile(_assetPath, _vsShaderEntryPoint, "vs_4_0", &VSBlob);

	if (FAILED(hr)) {
		
		WSS_CLASS_ERR_LOG("(Vertex Shader: %ls)The shader file cannot be compiled. Please run this executable"
			" from the directory that contains the FX file.\n", _assetPath.GetData());
		return hr;
	}

	hr =
		ExternalUtils::CompileShaderFromFile(_assetPath, _psShaderEntryPoint, "ps_4_0", &PSBlob);

	if (FAILED(hr)) {
		WSS_LOG(L"Warning: (Pixel Shader: %ls)The shader file cannot be compiled.  Please run this executable \
			from the directory that contains the FX file.\n", _assetPath.GetData());
		VSBlob->Release();
		VSBlob = nullptr;
		return hr;
	}

	// Reflection start
	{
		WRL::ComPtr<ID3D11ShaderReflection> VertexShaderReflection;
		WRL::ComPtr<ID3D11ShaderReflection> PixelShaderReflection;



		hr = D3DReflect(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)VertexShaderReflection.GetAddressOf());
		if (FAILED(hr)) {
			VSBlob->Release();
			PSBlob->Release();
			WSS_LOG(L"Error(SSShaderAsset::CompileShader()): (Vertex Shader: %ls) Reflection failed.\n", _assetPath.GetData());
			return E_FAIL;
		}

		hr = D3DReflect(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)PixelShaderReflection.GetAddressOf());
		if (FAILED(hr)) {
			VSBlob->Release();
			PSBlob->Release();
			WSS_LOG(L"Error(SSShaderAsset::CompileShader()): (Pixel Shader: %ls) Reflection failed.\n", _assetPath.GetData());
			return E_FAIL;
		}


		D3D11_SHADER_DESC VSShaderDesc;
		VertexShaderReflection->GetDesc(&VSShaderDesc);

		D3D11_SHADER_DESC PSShaderDesc;
		PixelShaderReflection->GetDesc(&PSShaderDesc);



		// 01. Input Layout Reflection
		layoutElemCount = VSShaderDesc.InputParameters;
		uint32 byteOffset = 0;
		for (uint8 i = 0; i < layoutElemCount; i++) {
			D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
			hr = VertexShaderReflection->GetInputParameterDesc(i, &paramDesc);

			if (FAILED(hr)) {
				SS_LOG("Error(SSShaderAsset::CompileShader): Input assembly reflection failed");
				return hr;
			}

			strcpy_s(SemanticStringList[i], paramDesc.SemanticName);
			LayoutDescArray[i].SemanticName = SemanticStringList[i];
			LayoutDescArray[i].SemanticIndex = paramDesc.SemanticIndex;
			LayoutDescArray[i].InputSlot = 0;
			LayoutDescArray[i].AlignedByteOffset = byteOffset;
			LayoutDescArray[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			LayoutDescArray[i].InstanceDataStepRate = 0;

			constexpr char SEMANTIC_POSITION[] = "POSITION";

			if (strcmp(paramDesc.SemanticName, SEMANTIC_POSITION) == 0) {
				LayoutDescArray[i].Format = DXGI_FORMAT_R32G32B32_FLOAT;
				byteOffset += 16; /// TODO: 이거 계산하는 로직 다시 짜자
			}
			else if (paramDesc.Mask == 1) {
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)		LayoutDescArray[i].Format = DXGI_FORMAT_R32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)	LayoutDescArray[i].Format = DXGI_FORMAT_R32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) LayoutDescArray[i].Format = DXGI_FORMAT_R32_FLOAT;
				byteOffset += 4;
			}
			else if (paramDesc.Mask <= 3) {
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)		LayoutDescArray[i].Format = DXGI_FORMAT_R32G32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)	LayoutDescArray[i].Format = DXGI_FORMAT_R32G32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) LayoutDescArray[i].Format = DXGI_FORMAT_R32G32_FLOAT;
				byteOffset += 8;
			}
			else if (paramDesc.Mask <= 7) {

				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)		LayoutDescArray[i].Format = DXGI_FORMAT_R32G32B32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)	LayoutDescArray[i].Format = DXGI_FORMAT_R32G32B32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) LayoutDescArray[i].Format = DXGI_FORMAT_R32G32B32_FLOAT;
				byteOffset += 16;
			}
			else if (paramDesc.Mask <= 15) {
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)		LayoutDescArray[i].Format = DXGI_FORMAT_R32G32B32A32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)	LayoutDescArray[i].Format = DXGI_FORMAT_R32G32B32A32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) LayoutDescArray[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				byteOffset += 16;

			}

		}

		// 02. VSConstantBuffer reflecction
		ShaderReflection.VSConstBufferNum = VSShaderDesc.ConstantBuffers;
		for (int i = 0; i < ShaderReflection.VSConstBufferNum; i++) {
			ID3D11ShaderReflectionConstantBuffer* CBReflection =
				VertexShaderReflection->GetConstantBufferByIndex(i);

			D3D11_SHADER_BUFFER_DESC bufferDesc;
			D3D11_SHADER_INPUT_BIND_DESC bufferBindDesc;

			hr = CBReflection->GetDesc(&bufferDesc);
			if (FAILED(hr)) {
				WSS_LOG(L"Error(SSShaderAsset::CompileShader()): (Vertex Shader: %ls) Vertex shader const buffer reflection failed.\n", _assetPath.GetData());
				return hr;
			}

			hr = VertexShaderReflection->GetResourceBindingDescByName(bufferDesc.Name, &bufferBindDesc);
			if (FAILED(hr)) {
				WSS_LOG(L"Error(SSShaderAsset::CompileShader()): (Vertex Shader: %ls) Vertex shader const buffer reflection failed.\n", _assetPath.GetData());
				return hr;
			}

			int BufferSize = ShaderReflection.VSCBReflectionInfo[i].CBSize = bufferDesc.Size;
			int SlotIdx = ShaderReflection.VSCBReflectionInfo[i].CBSlotIdx = bufferBindDesc.BindPoint;
			ShaderReflection.VSCBReflectionInfo[i].bCBUsedByVSShader = true;
			int nameLen = strlen(bufferDesc.Name) + 1;
			strcpy_s(ShaderReflection.VSCBReflectionInfo[i].CBName, nameLen, bufferDesc.Name);

			assert(BufferSize > 0);

			if (ShaderReflection.EntireCBReflectionInfo[SlotIdx].CBSize == INVALID_BUFFER_SIZE) {

				ShaderReflection.EntireCBReflectionInfo[SlotIdx]
					= ShaderReflection.VSCBReflectionInfo[i];


				if (SlotIdx > ShaderReflection.ConstBufferSlotMax) {
					ShaderReflection.ConstBufferSlotMax = SlotIdx;
				}
				ShaderReflection.EntireConstBufferNum++;

			}


			SS_LOG("%s\n", bufferDesc.Name);
		}


		// 03. PSConstant buffer reflection
		ShaderReflection.PSConstBufferNum = PSShaderDesc.ConstantBuffers;
		for (int i = 0; i < ShaderReflection.PSConstBufferNum; i++) {
			ID3D11ShaderReflectionConstantBuffer* CBReflection =
				PixelShaderReflection->GetConstantBufferByIndex(i);

			D3D11_SHADER_BUFFER_DESC bufferDesc;
			D3D11_SHADER_INPUT_BIND_DESC bufferBindDesc;

			hr = CBReflection->GetDesc(&bufferDesc);
			if (FAILED(hr)) {
				WSS_LOG(L"Error(SSShaderAsset::CompileShader()): (Pixel Shader: %ls) Pixel shader const buffer reflection failed.\n", _assetPath.GetData());
				return hr;
			}

			hr = PixelShaderReflection->GetResourceBindingDescByName(bufferDesc.Name, &bufferBindDesc);
			if (FAILED(hr)) {
				WSS_LOG(L"Error(SSShaderAsset::CompileShader()): (Pixel Shader: %ls) Pixel shader const buffer reflection failed.\n", _assetPath.GetData());
				return hr;
			}

			int BufferSize = ShaderReflection.PSCBReflectionInfo[i].CBSize = bufferDesc.Size;
			int SlotIdx = ShaderReflection.PSCBReflectionInfo[i].CBSlotIdx = bufferBindDesc.BindPoint;
			ShaderReflection.PSCBReflectionInfo[i].bCBUsedByPSShader = true;
			int nameLen = strlen(bufferDesc.Name) + 1;
			strcpy_s(ShaderReflection.PSCBReflectionInfo[i].CBName, nameLen, bufferDesc.Name);

			assert(BufferSize > 0);

			if (ShaderReflection.EntireCBReflectionInfo[SlotIdx].CBSize == INVALID_BUFFER_SIZE) {

				ShaderReflection.EntireCBReflectionInfo[SlotIdx]
					= ShaderReflection.PSCBReflectionInfo[i];

				if (SlotIdx > ShaderReflection.ConstBufferSlotMax) {
					ShaderReflection.ConstBufferSlotMax = SlotIdx;
				}
				ShaderReflection.EntireConstBufferNum++;

			}
			else
				ShaderReflection.EntireCBReflectionInfo[SlotIdx].bCBUsedByPSShader = true;

			SS_LOG("%s\n", bufferDesc.Name);
		}

		// 04. PS Texture and Sampler reflection
		ShaderReflection.TexturePoolCount = 0;
		ShaderReflection.SamplerCount = 0;

		for (uint32 i = 0; i < PSShaderDesc.BoundResources; i++) {

			D3D11_SHADER_INPUT_BIND_DESC bufdesc;
			hr = PixelShaderReflection->GetResourceBindingDesc(i, &bufdesc);


			if (FAILED(hr)) {
				WSS_LOG(L"Error(SSShaderAsset::CompileShader()): (Pixel Shader: %ls) Pixel shader texture reflection failed.\n", _assetPath.GetData());
				return hr;
			}

			switch (bufdesc.Type) {
			case D3D_SIT_TEXTURE: ShaderReflection.TexturePoolCount++; break;
			case D3D_SIT_SAMPLER: ShaderReflection.SamplerCount++; break;

			default: break;
			}
		}


	}


	_curStage = ShaderAssetInstanceStage::Compiled;
	return S_OK;
}


HRESULT SSShaderAsset::InstantiateShader(ID3D11Device* InDevice)
{
	switch (_curStage) {
	case ShaderAssetInstanceStage::JustCreated:
		WSS_LOG(L"Warning(SSShaderAsset::InstantiateShader): (Shader Name: %ls)Shader compile needed\n", _assetPath.GetData());
		return E_FAIL;
	case ShaderAssetInstanceStage::Compiled:
		break;
	case ShaderAssetInstanceStage::Instantiated:
		WSS_LOG(L"Warning(SSShaderAsset::InstantiateShader): (Shader Name: %ls)Shader is already instiantiated\n", _assetPath.GetData());
		return E_FAIL;
	}

	HRESULT hr = InDevice->
		CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), nullptr, &VertexShader);

	if (FAILED(hr)) {
		WSS_LOG(L"Error(SSShaderAsset::InstantiateShader): Vertex Shader[%ls] instantiate failed\n", _assetPath.GetData());
		return hr;
	}

	hr = InDevice->
		CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), nullptr, &PixelShader);

	if (FAILED(hr)) {
		VertexShader->Release();
		WSS_LOG(L"Error(SSShaderAsset::InstantiateShader): Pixel Shader[%ls] instantiate failed\n", _assetPath.GetData());
		return hr;
	}

	hr = InDevice->
		CreateInputLayout(LayoutDescArray, layoutElemCount, VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), &InputLayout);

	if (FAILED(hr)) {
		WSS_LOG(L"Error(SSShaderAsset::InstantiateShader): Shader[%ls] Create input layout failed\n", _assetPath.GetData());
		return hr;
	}

	VSBlob->Release();
	VSBlob = nullptr;
	PSBlob->Release();
	PSBlob = nullptr;


	_curStage = ShaderAssetInstanceStage::Instantiated;
	return S_OK;
}




void SSShaderAsset::BindShaderAsset(ID3D11DeviceContext* _deviceContext) const
{
	if (_curStage < ShaderAssetInstanceStage::Instantiated) {
		WSS_LOG(L"Warning[%ls]: Shader is not initalized completely. CurState: %d\n"
			, _assetPath.GetData(), _curStage);
		return;
	}

	_deviceContext->IASetInputLayout(InputLayout);
	_deviceContext->VSSetShader(VertexShader, nullptr, 0);
	_deviceContext->PSSetShader(PixelShader, nullptr, 0);

}

void SSShaderAsset::Release()
{
	switch (_curStage) {

	case ShaderAssetInstanceStage::Instantiated:

		InputLayout->Release();
		VertexShader->Release();
		PixelShader->Release();

	case ShaderAssetInstanceStage::Compiled:

		if (PSBlob != nullptr) PSBlob->Release();
		if (VSBlob != nullptr) VSBlob->Release();

	case ShaderAssetInstanceStage::JustCreated:
	default:
		break;
	}
}
