

#pragma once

#ifdef _WINDOWS
#include <DirectXMath.h>

#include "SSVector.h"

using namespace DirectX;

typedef long long int			int64;
typedef int						int32;
typedef short					int16;
typedef char					int8;

typedef unsigned long long		uint64;
typedef unsigned int			uint32;
typedef unsigned short			uint16;
typedef unsigned char			uint8;




struct Transform {
	Vector4f Position;
	Vector4f Rotation;
	Vector4f Scale;

	
	__forceinline XMMATRIX AsMatrix() {
		return
		XMMatrixAffineTransformation(
			Scale.SimdVec,
			Rotation.SimdVec,
			XMQuaternionRotationRollPitchYawFromVector(Rotation.SimdVec),
			Position.SimdVec
		);
		
	}
};

#endif