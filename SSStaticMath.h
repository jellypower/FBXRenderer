#pragma once
#include <DirectXMath.h>

using namespace DirectX;

class SSStaticMath
{
public:
	__forceinline static float RadToDegrees(float InRad);
	__forceinline static float DegToRadians(float InDeg);
};

inline float SSStaticMath::RadToDegrees(float InRad)
{
	return InRad / XM_PI * 180.f;
}

inline float SSStaticMath::DegToRadians(float InDeg)
{
	return InDeg / 180.f * XM_PI;
}
