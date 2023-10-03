#pragma once
#include <windows.h>
#include <d3d11.h>

#include "SSNativeTypes.h"

#include "SSShaderAsset.h"

class SSShaderAssetManager
{
public:
	void Init(/*TODO: file* ShaderList*/);
	void Release();

	/** TODO: Register/UnRegister shader functionality*/
	/** TODO: Read file from functionality must be seperated as ExternalUtils*/
	HRESULT CompileAllShader();

	HRESULT InstantiateShader(uint8 ShaderName /*TODO: change as hash*/, ID3D11Device* InDevice);
	HRESULT InstantiateAllShader(ID3D11Device* InDevice);
	

	void ReleaseShader(uint8 ShaderName /*TPDP" change as hash*/);
	void ReleaseAllShader();

	/** TODO: replace as map with hash string*/
	inline SSShaderAsset* GetShaderAsset(uint8 idx) {
		return ShaderList[idx];
	}

private:
	/** TODO: replace as map with hash string*/
	static constexpr uint8 DEFAULT_POOL_SIZE = 10;
	uint8 ShaderPoolCount = 0;
	SSShaderAsset** ShaderList;
};