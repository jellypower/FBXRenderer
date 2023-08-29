#pragma once

#include <windows.h>
#include <d3d11.h>

#include "SSNativeTypes.h"

#include "SSShaderReflectionForMaterial.h"




enum class ShaderAssetInstanceStage {
	JustCreated = 0,
	Compiled = 1,
	Instantiated
};

// TODO: ShaderMetaInfo를 활용해서 실제 셰이더의 자료형이라던가, 버퍼 크기라던가 저장하기.
struct ShaderMetaInfo {

};

class SSShaderAsset
{

public:
	SSShaderAsset(const WCHAR* InShaderName, LPCSTR szVSEntryPoint, LPCSTR szPSEntryPoint, LPCSTR szShaderModel);
	HRESULT CompileShader();
	
	HRESULT InstantiateShader(ID3D11Device* InDevice);
	


	// TODO: Shader에 이름 지어주고 GetName함수 만들어주기

	// TODO: 해당 셰이더가 어떤 콘스탄트 버퍼, 텍스쳐를 사용하는지에 대한 메타 정보를 담아두는 데이터 별도 제작

	// TODO: 지금은 셰이더당 버퍼가 있는데 셰이더당 버퍼가 아니라 메테리얼당 버퍼가 있어야 한다

	__forceinline void UpdateShaderWVPTransform(ID3D11DeviceContext* InDeviceContext, const XMMATRIX& InMatrix);
	void UpdateAllShaderCBData(ID3D11DeviceContext* InDeviceContext, void** InConstBufferData, uint8 InConstBufferCount);
	void BindShaderAsset(ID3D11DeviceContext* DeviceContext);


	~SSShaderAsset();

public:
	__forceinline ShaderAssetInstanceStage GetShaderInstanceStage() const { return CurStage; }

		const __forceinline SSShaderReflectionForMaterial*
			GetShaderReflectionPointer() const { return &ShaderReflection;  }

private:
	ShaderAssetInstanceStage CurStage = ShaderAssetInstanceStage::JustCreated;

private:
	ID3DBlob* VSBlob = nullptr;
	ID3DBlob* PSBlob = nullptr;
	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;

	WCHAR ShaderName[SHADER_FILE_NAME_MAX_LEN] = {};
	CHAR VSEntryPointName[VS_SHADER_ENTRY_NAME_MAX_LEN] = {};
	CHAR PSEntryPointName[PS_SHADER_ENTRY_NAME_MAX_LEN] = {};

	D3D11_INPUT_ELEMENT_DESC LayoutDescArray[LAYOUT_NUM_MAX] = {};
	CHAR SemanticStringList[LAYOUT_NUM_MAX][LAYOUT_SEMANTIC_NAME_LEN_MAX];
	uint8 layoutElemCount = 0;


	// TODO: 지금은 단순 배열로 만들지만 Constant Buffer, Texture List를 Hashstring을 사용한 Map으로 변경하자
	// TODO: Shader CBParamType enum 만들기, Shader TextureParamType enum 만들기
	

	ID3D11Buffer* ConstantBuffers[CONSTANT_BUFFER_COUNT_MAX] = {};
	ID3D11Buffer* VSConstantBuffers[CONSTANT_BUFFER_COUNT_MAX] = {};
	ID3D11Buffer* PSConstantBuffers[CONSTANT_BUFFER_COUNT_MAX] = {};

	SSShaderReflectionForMaterial ShaderReflection;


};

inline void SSShaderAsset::UpdateShaderWVPTransform(ID3D11DeviceContext* InDeviceContext, const XMMATRIX& InMatrix)
{
	InDeviceContext->UpdateSubresource(ConstantBuffers[WVP_TRANSFOMRM_IDX], 0, nullptr, &InMatrix, 0, 0);
}
