#pragma once

#include "SSNativeTypes.h"
#include "SSStaticMath.h"

#include <d3d11.h>
#include <windows.h>


class SSCamera
{

public:

	SSCamera();

	void UpdateResolutionWithClientRect(ID3D11Device* InDevice, HWND InHwnd);
	Transform& GetTransform() { return transform;  }

public:

	__forceinline XMMATRIX GetViewMatrix();
	__forceinline XMMATRIX GetProjectionMatrix();
	__forceinline XMMATRIX GetViewProjMatrix();

	__forceinline void SetFOVWithRadians(float InRadians);
	__forceinline void SetFOVWithDegrees(float InDegrees);
	__forceinline float GetRadianFOV();
	__forceinline float GetDegFOV();

	__forceinline void SetNearZ(float InValue);
	__forceinline void SetFarZ(float InValue);
	__forceinline void SetNearFarZ(float InNearZ, float InFarZ);

	__forceinline float GetNearZ();
	__forceinline float GetFarZ();

private:
	Transform transform;
	RECT ScreenRect = { 0 };
	
	float FOVRadY = 0;
	float NearZ = 0;
	float FarZ = 0;

};




XMMATRIX SSCamera::GetViewMatrix()
{
	XMVECTOR Eye = transform.Position.SimdVec;
	XMVECTOR At = 
		XMVector4Transform(Vector4f::Forward.SimdVec, 
			XMMatrixRotationRollPitchYawFromVector(transform.Rotation.SimdVec));
	XMVECTOR Up = 
		XMVector4Transform(Vector4f::Up.SimdVec,
			XMMatrixRotationRollPitchYawFromVector(transform.Rotation.SimdVec));
	
	
	return XMMatrixLookAtLH(Eye, At, Up);
}

XMMATRIX SSCamera::GetProjectionMatrix()
{
	UINT width = ScreenRect.right - ScreenRect.left;
	UINT height = ScreenRect.bottom - ScreenRect.top;

	return XMMatrixPerspectiveFovLH(FOVRadY, width / (FLOAT)height, 0.01f, 100.0f);;
}

inline XMMATRIX SSCamera::GetViewProjMatrix()
{
	return GetViewMatrix() * GetProjectionMatrix();
}

inline void SSCamera::SetFOVWithRadians(float InRadians)
{
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
