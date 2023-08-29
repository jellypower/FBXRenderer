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

// TODO: ShaderMetaInfo�� Ȱ���ؼ� ���� ���̴��� �ڷ����̶����, ���� ũ������ �����ϱ�.
struct ShaderMetaInfo {

};

class SSShaderAsset
{

public:
	SSShaderAsset(const WCHAR* InShaderName, LPCSTR szVSEntryPoint, LPCSTR szPSEntryPoint, LPCSTR szShaderModel);
	HRESULT CompileShader();
	
	HRESULT InstantiateShader(ID3D11Device* InDevice);
	


	// TODO: Shader�� �̸� �����ְ� GetName�Լ� ������ֱ�

	// TODO: �ش� ���̴��� � �ܽ�źƮ ����, �ؽ��ĸ� ����ϴ����� ���� ��Ÿ ������ ��Ƶδ� ������ ���� ����

	// TODO: ������ ���̴��� ���۰� �ִµ� ���̴��� ���۰� �ƴ϶� ���׸���� ���۰� �־�� �Ѵ�

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


	// TODO: ������ �ܼ� �迭�� �������� Constant Buffer, Texture List�� Hashstring�� ����� Map���� ��������
	// TODO: Shader CBParamType enum �����, Shader TextureParamType enum �����
	

	ID3D11Buffer* ConstantBuffers[CONSTANT_BUFFER_COUNT_MAX] = {};
	ID3D11Buffer* VSConstantBuffers[CONSTANT_BUFFER_COUNT_MAX] = {};
	ID3D11Buffer* PSConstantBuffers[CONSTANT_BUFFER_COUNT_MAX] = {};

	SSShaderReflectionForMaterial ShaderReflection;


};

inline void SSShaderAsset::UpdateShaderWVPTransform(ID3D11DeviceContext* InDeviceContext, const XMMATRIX& InMatrix)
{
	InDeviceContext->UpdateSubresource(ConstantBuffers[WVP_TRANSFOMRM_IDX], 0, nullptr, &InMatrix, 0, 0);
}
