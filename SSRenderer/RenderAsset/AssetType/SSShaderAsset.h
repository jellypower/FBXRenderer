#pragma once

#include <windows.h>
#include <d3d11.h>

#include "SSAssetBase.h"
#include "SSEngineDefault/SSNativeTypes.h"

#include "SSShaderReflectionForMaterial.h"




enum class ShaderAssetInstanceStage {
	JustCreated = 0, // 단순 생성만 됨
	Compiled = 1, // 컴파일이 완료됨
	Instantiated // GPU에서까지 사용 가능함
};


class SSShaderAsset : public SSAssetBase
{

public:
	SSShaderAsset(const char* InShaderName, const utf16* InShaderPath, LPCSTR szVSEntryPoint, LPCSTR szPSEntryPoint, LPCSTR szShaderModel);

	HRESULT CompileShader();
	HRESULT InstantiateShader(ID3D11Device* InDevice);
	void Release();

	void BindShaderAsset(ID3D11DeviceContext* _deviceContext) const;



public:
	FORCEINLINE ShaderAssetInstanceStage GetShaderInstanceStage() const { return _curStage; }

	const FORCEINLINE SSShaderReflectionForMaterial* GetShaderReflectionPtr() const { return &ShaderReflection; }

private:
	ShaderAssetInstanceStage _curStage = ShaderAssetInstanceStage::JustCreated;

private:
	ID3DBlob* VSBlob = nullptr;
	ID3DBlob* PSBlob = nullptr;
	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;

	SS::FixedStringA<VS_SHADER_ENTRY_NAME_MAX_LEN> _vsShaderEntryPoint;
	SS::FixedStringA<PS_SHADER_ENTRY_NAME_MAX_LEN> _psShaderEntryPoint;

	D3D11_INPUT_ELEMENT_DESC LayoutDescArray[LAYOUT_NUM_MAX] = {};
	CHAR SemanticStringList[LAYOUT_NUM_MAX][LAYOUT_SEMANTIC_NAME_LEN_MAX];
	uint8 layoutElemCount = 0;

	SSShaderReflectionForMaterial ShaderReflection;


};