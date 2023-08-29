#pragma once

#ifdef _WINDOWS
#include <DirectXMath.h>

using namespace DirectX;

#pragma region Vector4f
struct Vector4f {

	Vector4f();
	Vector4f(XMVECTOR InXMVECTOR);
	Vector4f(float InX, float InY, float InZ, float InW);

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
	
	static const Vector4f Forward;
	static const Vector4f Back;
	static const Vector4f Up;
	static const Vector4f Down;
	static const Vector4f Left;
	static const Vector4f Right;

};

__forceinline const Vector4f& operator+(const Vector4f& lhs, const Vector4f& rhs) {
	return lhs.SimdVec + rhs.SimdVec;
}

__forceinline const Vector4f& operator-(const Vector4f& lhs, const Vector4f& rhs) {
	return lhs.SimdVec - rhs.SimdVec;
}

__forceinline const Vector4f& operator*(const Vector4f& lhs, const Vector4f& rhs) {
	return lhs.SimdVec * rhs.SimdVec;
}

__forceinline const Vector4f& operator*(const Vector4f& lhs, float rhs) {
	return lhs.SimdVec * rhs;
}

__forceinline const Vector4f& operator*(float lhs, const Vector4f& rhs) {
	return lhs * rhs.SimdVec;
}

__forceinline const Vector4f& operator/(const Vector4f& lhs, float rhs) {
	return lhs / rhs;
}

__forceinline const Vector4f& operator-(const Vector4f& inVal) {
	return -inVal.SimdVec;
}

#pragma endregion

#endif