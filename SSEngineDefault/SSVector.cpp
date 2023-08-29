#include "SSVector.h"

Vector4f const Vector4f::Forward = Vector4f(0, 0, 1, 0);
Vector4f const Vector4f::Back = Vector4f(0, 0, -1, 0);
Vector4f const Vector4f::Up = Vector4f(0, 1, 0, 0);
Vector4f const  Vector4f::Down = Vector4f(0, -1, 0, 0);
Vector4f const Vector4f::Left = Vector4f(-1, 0, 0, 0);
Vector4f const Vector4f::Right = Vector4f(1, 0, 0, 0);

Vector4f::Vector4f() {
	SimdVec = { 0 };
}

Vector4f::Vector4f(XMVECTOR InXMVECTOR) {
	SimdVec = InXMVECTOR;
}

Vector4f::Vector4f(float InX, float InY, float InZ, float InW) {
	X = InX;
	Y = InY;
	Z = InZ;
	W = InW;
}
