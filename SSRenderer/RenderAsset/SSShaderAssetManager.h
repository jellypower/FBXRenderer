#pragma once
#include <windows.h>
#include <d3d11.h>

#include "SSEngineDefault/SSEngineDefault.h"

#include "AssetType/SSShaderAsset.h"
#include "SSEngineDefault/SSContainer/PooledList.h"
#include "SSEngineDefault/SSContainer/StringHashMapA.h"

class SSShaderAssetManager
{
	friend class SSRenderer;
	SS_DECLARE_AS_SINGLETON(SSShaderAssetManager)

private:
	static FORCEINLINE void Instantiate(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed) {
		if (g_instance != nullptr) {
			assert(false);
			return;
		}
		g_instance = DBG_NEW SSShaderAssetManager(poolCapacity, hashCapacity, hashCollisionLimit, hashSeed);
	}

public:
	static FORCEINLINE SSShaderAsset* FindShaderWithName(const char* name)
	{
		assert(g_instance != nullptr);
		return g_instance->FindShaderWithNameInternal(name);
	}
	static FORCEINLINE SSShaderAsset* GetShaderWithIdx(uint32 idx)
	{
		assert(g_instance != nullptr);
		return g_instance->GetShaderAssetWithIdxInternal(idx);
	}

	static constexpr char SSDefaultPbrShaderName[] = "SSDefaultPbr";



private:
	SS::PooledList<SSShaderAsset*> _assetPool;
	SS::StringHashMapA<uint32> _assetHashMap;

public:

	void InsertNewShaderAsset(SSShaderAsset* newShader);
	void LoadNewShaderTemp();

private:
	SSShaderAssetManager(uint32 poolCapacity, uint32 hashCapacity, uint32 hashCollisionLimit, uint64 hashSeed);
	~SSShaderAssetManager();

	FORCEINLINE SSShaderAsset* GetShaderAssetWithIdxInternal(uint32 idx){ return _assetPool[idx]; }
	SSShaderAsset* FindShaderWithNameInternal(const char* name);

	HRESULT CompileAllShader();

	HRESULT InstantiateShaderWitIdx(uint32 ShaderName, ID3D11Device* InDevice);
	HRESULT InstantiateAllShader(ID3D11Device* InDevice);

	void ReleaseShaderWithIdx(uint32 idx /*TODO: change as hash*/);
	void ReleaseAllShader();
};
