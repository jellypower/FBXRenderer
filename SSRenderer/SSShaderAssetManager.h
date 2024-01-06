#pragma once
#include <windows.h>
#include <d3d11.h>

#include "SSEngineDefault/SSEngineDefault.h"

#include "SSShaderAsset.h"

class SSShaderAssetManager
{
	friend class SSRenderer;
	SS_DECLARE_AS_SINGLETON(SSShaderAssetManager)
private:
	static FORCEINLINE void Instantiate(uint32 poolCapacity) {
		if (g_instance != nullptr) {
			assert(false);
			return;
		}
		g_instance = DBG_NEW SSShaderAssetManager(poolCapacity);
	}


private:
	uint32 _shaderPoolCount = 0;
	uint32 _shaderPoolCapacity = 0;
	SSShaderAsset** ShaderList;

public:
	FORCEINLINE SSShaderAsset* GetShaderAsset(uint32 idx) {
		assert(idx < _shaderPoolCount);
		return ShaderList[idx];
	}
	void LoadNewShaderTemp();

private:
	SSShaderAssetManager(uint32 poolCapacity);
	~SSShaderAssetManager();

	HRESULT CompileAllShader();

	HRESULT InstantiateShader(uint32 ShaderName, ID3D11Device* InDevice);
	HRESULT InstantiateAllShader(ID3D11Device* InDevice);

	void ReleaseShader(uint32 ShaderName /*TODO: change as hash*/);
	void ReleaseAllShader();
};