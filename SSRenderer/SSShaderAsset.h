#pragma once

#include <windows.h>
#include <d3d11.h>

#include "SSEngineDefault/SSNativeTypes.h"

#include "SSShaderReflectionForMaterial.h"




enum class ShaderAssetInstanceStage {
	JustCreated = 0,
	Compiled = 1,
	Instantiated
};


class SSShaderAsset
{

public:
	SSShaderAsset(const WCHAR* InShaderName, LPCSTR szVSEntryPoint, LPCSTR szPSEntryPoint, LPCSTR szShaderModel);
	HRESULT CompileShader();
	
	HRESULT InstantiateShader(ID3D11Device* InDevice);
	
	void Release();

	// TODO: Shader�� �̸� �����ְ� GetName�Լ� ������ֱ�

	// TODO: �ش� ���̴��� � �ܽ�źƮ ����, �ؽ��ĸ� ����ϴ����� ���� ��Ÿ ������ ��Ƶδ� ������ ���� ����

	// TODO: ������ ���̴��� ���۰� �ִµ� ���̴��� ���۰� �ƴ϶� ���׸���� ���۰� �־�� �Ѵ�

	void BindShaderAsset(ID3D11DeviceContext* _deviceContext);



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

	SSShaderReflectionForMaterial ShaderReflection;


};