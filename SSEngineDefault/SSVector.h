#pragma once

#ifdef _WINDOWS
#include <DirectXMath.h>
#include "SSNativeKeywords.h"

using namespace DirectX;

struct Vector4f {
	union {
		XMVECTOR SimdVec;

		struct {
			float X;
			float Y;
			float Z;
			float W;
		};
		float At[4];
	};

	Vector4f();
	Vector4f(XMVECTOR InXMVECTOR);
	Vector4f(float InX, float InY, float InZ, float InW);
	FORCEINLINE Vector4f GetNormalized();
	FORCEINLINE float Get3DLength();
	FORCEINLINE float Get3DSqrLength();

	static const Vector4f Forward;
	static const Vector4f Back;
	static const Vector4f Up;
	static const Vector4f Down;
	static const Vector4f Left;
	static const Vector4f Right;
	static const Vector4f Zero;
	static const Vector4f One;
	static const Vector4f Half;
};

FORCEINLINE Vector4f operator+(const Vector4f& lhs, const Vector4f& rhs) {
	return lhs.SimdVec + rhs.SimdVec;
}

FORCEINLINE Vector4f operator-(const Vector4f& lhs, const Vector4f& rhs) {
	return lhs.SimdVec - rhs.SimdVec;
}

FORCEINLINE Vector4f operator*(const Vector4f& lhs, const Vector4f& rhs) {
	return lhs.SimdVec * rhs.SimdVec;
}

FORCEINLINE Vector4f operator*(const Vector4f& lhs, float rhs) {
	return lhs.SimdVec * rhs;
}

FORCEINLINE Vector4f operator*(float lhs, const Vector4f& rhs) {
	return lhs * rhs.SimdVec;
}

FORCEINLINE Vector4f operator/(const Vector4f& lhs, float rhs) {
	return lhs.SimdVec / rhs;
}

FORCEINLINE Vector4f operator-(const Vector4f& inVal) {
	return -inVal.SimdVec;
}

FORCEINLINE Vector4f Vector4f::GetNormalized()
{
	return SimdVec / Get3DLength();
}

FORCEINLINE float Vector4f::Get3DLength()
{
	return sqrt(Get3DSqrLength());
}

FORCEINLINE float Vector4f::Get3DSqrLength()
{
	return XMVector3Length(SimdVec).m128_f32[0];
}






struct Vector2f {
	float X;
	float Y;

	Vector2f();
	Vector2f(float InX, float InY);

	static const Vector2f Zero;
};

FORCEINLINE Vector2f operator-(const Vector2f& lhs, const Vector2f& rhs) {
	return Vector2f(lhs.X - rhs.X, lhs.Y - rhs.Y);
}


struct Quaternion {
	union {
		XMVECTOR SimdVec;
		struct {
			float X;
			float Y;
			float Z;
			float W;
		};
		float At[4];
	};

	Quaternion();
	Quaternion(__m128 InSimdVector);
	FORCEINLINE XMMATRIX AsMatrix() const { return XMMatrixRotationQuaternion(SimdVec); }

	static Quaternion FromEulerRotation(Vector4f eulerRotation);
	static Quaternion FromLookDirect(Vector4f lookDirection, Vector4f upDirection = Vector4f::Up);
	static Quaternion RotateAxisAngle(Quaternion CurRotation, Vector4f Axis, float angle);
};



struct Vector2i32 {
	int32 X;
	int32 Y;

	Vector2i32();
	Vector2i32(int32 InX, int32 InY);

	static const Vector2i32 Zero;

};

FORCEINLINE Vector2i32 operator+(const Vector2i32 lhs, const Vector2i32 rhs) {
	return Vector2i32(lhs.X + rhs.X, lhs.Y + rhs.Y);
}

FORCEINLINE Vector2i32 operator-(const Vector2i32 lhs, const Vector2i32 rhs) {
	return Vector2i32(lhs.X - rhs.X, lhs.Y - rhs.Y);
}



struct Vector2ui32 {
	uint32 X;
	uint32 Y;

	Vector2ui32();
	Vector2ui32(uint32 InX, uint32 InY);
};



struct Transform {
	Vector4f Position;
	Quaternion Rotation;
	Vector4f Scale;

	Transform();
	Transform(Vector4f InPos, Quaternion InRot, Vector4f InScale);


	FORCEINLINE XMMATRIX AsMatrix() const {
		return
			XMMatrixAffineTransformation(
				Scale.SimdVec,		// 스케일
				{ 0 },	// 피벗
				Rotation.SimdVec,	// 회전
				Position.SimdVec	// 위치
			);

	}
	FORCEINLINE Vector4f GetForward() const
	{
		return XMVector3Rotate(Vector4f::Forward.SimdVec, Rotation.SimdVec);
	};
	FORCEINLINE Vector4f GetBackward() const
	{
		return XMVector3Rotate(Vector4f::Back.SimdVec, Rotation.SimdVec);
	}
	FORCEINLINE Vector4f GetUp() const
	{
		return XMVector3Rotate(Vector4f::Up.SimdVec, Rotation.SimdVec);
	}
	FORCEINLINE Vector4f GetDown() const
	{
		return XMVector3Rotate(Vector4f::Down.SimdVec, Rotation.SimdVec);
	}
	FORCEINLINE Vector4f GetLeft() const
	{
		return XMVector3Rotate(Vector4f::Left.SimdVec, Rotation.SimdVec);
	}
	FORCEINLINE Vector4f GetRight() const
	{
		return XMVector3Rotate(Vector4f::Right.SimdVec, Rotation.SimdVec);
	}

};



#endif