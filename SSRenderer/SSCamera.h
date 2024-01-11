#pragma once

#include "SSEngineDefault/SSNativeTypes.h"
#include "SSPlaceableObject.h"
#include "SSStaticMath.h"

#include <d3d11.h>
#include <windows.h>


constexpr float CAM_FOV_MIN = 0.01f;
constexpr float CAM_FOV_MAX = XM_PI * 0.99f;

class SSCamera : public SSPlaceableObject
{

private:
	RECT ScreenRect = { 0 };

	float FOVRadY = 0;
	float NearZ = 0;
	float FarZ = 0;

public:

	SSCamera();
	virtual ~SSCamera() override = default;

	void UpdateResolutionWithClientRect(ID3D11Device* InDevice, HWND InHwnd);
	Transform& GetTransform() { return _transform;  }


	FORCEINLINE XMMATRIX GetViewMatrix();
	FORCEINLINE XMMATRIX GetProjectionMatrix();
	FORCEINLINE XMMATRIX GetViewProjMatrix();

	FORCEINLINE void SetFOVWithRadians(float InRadians);
	FORCEINLINE void SetFOVWithDegrees(float InDegrees);
	FORCEINLINE float GetRadianFOV();
	FORCEINLINE float GetDegFOV();

	FORCEINLINE void SetNearZ(float InValue);
	FORCEINLINE void SetFarZ(float InValue);
	FORCEINLINE void SetNearFarZ(float InNearZ, float InFarZ);

	FORCEINLINE float GetNearZ();
	FORCEINLINE float GetFarZ();
};




XMMATRIX SSCamera::GetViewMatrix()
{
	XMVECTOR EyePos = _transform.Position.SimdVec;
	XMVECTOR Direction = _transform.GetForward().SimdVec;
	XMVECTOR Up = Vector4f::Up.SimdVec;
	
	return XMMatrixLookToLH(EyePos, Direction, _transform.GetUp().SimdVec);
}

XMMATRIX SSCamera::GetProjectionMatrix()
{
	UINT width = ScreenRect.right - ScreenRect.left;
	UINT height = ScreenRect.bottom - ScreenRect.top;

	return XMMatrixPerspectiveFovLH(FOVRadY, width / (FLOAT)height, NearZ, FarZ);
}

inline XMMATRIX SSCamera::GetViewProjMatrix()
{
	return GetViewMatrix() * GetProjectionMatrix();
}

inline void SSCamera::SetFOVWithRadians(float InRadians)
{
	if (InRadians < CAM_FOV_MIN)
	{
		InRadians = CAM_FOV_MIN;
		return;
	}

	if (InRadians > CAM_FOV_MAX)
	{
		InRadians = CAM_FOV_MAX;
		return;
	}

	FOVRadY = InRadians;
}

inline void SSCamera::SetFOVWithDegrees(float InDegrees)
{
	FOVRadY = SSStaticMath::DegToRadians(InDegrees);
}

inline float SSCamera::GetRadianFOV()
{
	return FOVRadY;
}

inline float SSCamera::GetDegFOV()
{
	return SSStaticMath::RadToDegrees(FOVRadY);
}

inline void SSCamera::SetNearZ(float InValue)
{
	NearZ = InValue;
}

inline void SSCamera::SetFarZ(float InValue)
{
	FarZ = InValue;
}

inline void SSCamera::SetNearFarZ(float InNearZ, float InFarZ)
{
	NearZ = InNearZ;
	FarZ = InFarZ;
}

inline float SSCamera::GetNearZ()
{
	return NearZ;
}

inline float SSCamera::GetFarZ()
{
	return FarZ;
}
