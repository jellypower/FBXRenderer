#pragma once
#include <SSEngineDefault/SSNativeKeywords.h>
#include <DirectXMath.h>

using namespace DirectX;

namespace SSStaticMath
{

	FORCEINLINE float RadToDegrees(float InRad)
	{
		return InRad / XM_PI * 180.f;
	}

	FORCEINLINE float DegToRadians(float InDeg)
	{
		return InDeg / 180.f * XM_PI;
	}


	XMMATRIX InverseRigid(XMMATRIX InMatrix);
};
