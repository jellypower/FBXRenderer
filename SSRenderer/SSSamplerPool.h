#pragma once

#include <cassert>
#include <d3d11.h>

#include "SSEngineDefault/SSEngineDefault.h"

class SSRenderer;



enum class TexAddressMode
{
	Repeat = 0,
	MirroredRepeat = 1,
	ClampEdge,
	UserDefinedBorder,

	Count
};

enum class TexFilterMode
{
	Linear = 0,

	Count
};

class SSSamplerPool
{
	SS_DECLARE_AS_SINGLETON(SSSamplerPool);
public:
	static FORCEINLINE void Instantiate() {
		if (g_instance != nullptr) {
			assert(false);
			return;
		}
		g_instance = DBG_NEW SSSamplerPool();
	}




private:
	SSRenderer* _renderer;
	ID3D11SamplerState* _samplerCache[(uint32)TexFilterMode::Count][(uint32)TexAddressMode::Count][(uint32)TexAddressMode::Count] = { 0, };

public:
	~SSSamplerPool();

	FORCEINLINE void SetRenderer(SSRenderer* InRenderer) { _renderer = InRenderer; }
	ID3D11SamplerState* GetSampler(TexFilterMode filterMode, TexAddressMode AddressU, TexAddressMode AddressV);

};

