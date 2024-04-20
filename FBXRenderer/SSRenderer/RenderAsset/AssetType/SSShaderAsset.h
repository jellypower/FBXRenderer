#pragma once

#include <windows.h>
#include <d3d11.h>

#include "SSAssetBase.h"
#include "SSEngineDefault/SSNativeTypes.h"

#include "SSShaderReflectionForMaterial.h"
#include "SSEngineDefault/SSContainer/FixedList.h"


enum class ShaderAssetInstanceStage {
	JustCreated = 0, // 단순 생성만 됨
	Compiled = 1, // 컴파일이 완료됨
	Instantiated // GPU에서까지 사용 가능함
};


class SSShaderAsset : public SSAssetBase
{
private:
	ID3DBlob* VSBlob = nullptr;
	ID3DBlob* PSBlob = nullptr;
	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;

	SS::FixedStringA<VS_SHADER_ENTRY_NAME_MAX_LEN> _vsShaderEntryPoint;
	SS::FixedStringA<PS_SHADER_ENTRY_NAME_MAX_LEN> _psShaderEntryPoint;
	SS::FixedList<SS::FixedStringA<SHADER_DEFINE_STR_LEN_MAX>, SHADER_DEFINE_CNT_MAX> _shaderDefines;
	
	D3D_SHADER_MACRO _shaderMacros[SHADER_DEFINE_CNT_MAX + 1];

	D3D11_INPUT_ELEMENT_DESC LayoutDescArray[LAYOUT_NUM_MAX] = {};
	CHAR SemanticStringList[LAYOUT_NUM_MAX][LAYOUT_SEMANTIC_NAME_LEN_MAX];
	uint8 layoutElemCount = 0;

	SSShaderReflectionForMaterial ShaderReflection;


public:
	SSShaderAsset(const char* InShaderName, const utf16* InShaderPath, LPCSTR szVSEntryPoint, LPCSTR szPSEntryPoint, LPCSTR szShaderModel, uint32 argCnt=0, ...);
	

	HRESULT CompileShader();
	HRESULT InstantiateShader(ID3D11Device* InDevice);
	void Release();

	void BindShaderAsset(ID3D11DeviceContext* _deviceContext) const;



public:
	FORCEINLINE ID3D11InputLayout* GetInputLayout() const { return InputLayout; }
	FORCEINLINE ID3D11VertexShader* GetVertexShader() const { return VertexShader; }
	FORCEINLINE ID3D11PixelShader* GetPixelShader() const { return PixelShader;  }

	FORCEINLINE ShaderAssetInstanceStage GetShaderInstanceStage() const { return _curStage; }

	const FORCEINLINE SSShaderReflectionForMaterial* GetShaderReflectionPtr() const { return &ShaderReflection; }

private:
	ShaderAssetInstanceStage _curStage = ShaderAssetInstanceStage::JustCreated;

};