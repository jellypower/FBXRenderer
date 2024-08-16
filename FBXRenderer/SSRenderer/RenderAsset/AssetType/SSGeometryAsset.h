#pragma once
#include "SSEngineDefault/SSNativeTypes.h"

#include <d3d11.h>
#include <Windows.h>

#include "SSAssetBase.h"


constexpr uint32 SUBGEOM_COUNT_MAX = 8;

struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};

struct alignas(16) SSDefaultVertex {
	Vector4f Pos;
	Vector4f Normal;
	Vector4f Tangent;
	Vector2f Uv[2];
};

struct SSSkinnedVertex : SSDefaultVertex
{
	uint32 BoneIdx[4] = { 0, };
	float Weight[4] = { 0, };
};

enum class EMeshType
{
	None = 0,
	Rigid = 1,
	Skinned = 2
};


enum class EVertexUnit
{
	None,

	VertexPerPoint, // ���� position�� �ִ� ����� ���� 1���� ������ ����
	VertexPerPolygon, // �ϳ��� polygon(�ٰ���)���� ������ ����
};


class SSGeometryAsset : public SSAssetBase
{
	friend class SSFBXImporter;
private:
	EMeshType _meshType = EMeshType::None;
	EVertexUnit _vertexUnit = EVertexUnit::None; // ���ֱ�

	void* _vertexData = nullptr;
	uint32 _eachVertexDataSize = 0;
	uint32 _vertexCnt = 0;
	ID3D11Buffer* _vertexBuffer = nullptr;


	uint8 _subGeometryNum;
	uint32* _indexData = nullptr;
	uint32 _indexDataNum[SUBGEOM_COUNT_MAX] = { 0, };
	uint32 _indexDataStartIndex[SUBGEOM_COUNT_MAX] = { 0, };
	uint32 _wholeIndexDataNum = 0;
	ID3D11Buffer* _indexBuffer = nullptr;

	class SSSkeletonAsset* boundSkeletonAsset;


public:
	SSGeometryAsset();
	virtual ~SSGeometryAsset() override = default;

	void ReleaseSystemData();
	bool UsableOnSystem() const { return _vertexData != nullptr; }

	HRESULT UpdateDataOnGPU(ID3D11Device* InDevice);
	bool UsableOnGPU() const { return _vertexBuffer != nullptr; }
	void ReleaseGPUData();

	void BindGeometry(ID3D11DeviceContext* InDeviceContex, uint32 subGeomIdx = 0) const;

public:
	FORCEINLINE EMeshType GetMeshType() const { return _meshType; }
	FORCEINLINE uint8 GetSubGeometryNum() const { return _subGeometryNum; }
	FORCEINLINE uint32 GetIndexDataStartIndex(uint32 subGeomIdx = 0) const { return _indexDataStartIndex[subGeomIdx]; }
	FORCEINLINE uint32 GetIndexDataNum(uint32 subGeomIdx = 0) const { return _indexDataNum[subGeomIdx]; }

	FORCEINLINE ID3D11Buffer* const* GetVertexBufferPtr() const { return &_vertexBuffer; }
	FORCEINLINE ID3D11Buffer* GetIndexBuffer() const { return _indexBuffer; }
	FORCEINLINE uint32 GetEachVertexDataSize() const { return _eachVertexDataSize; }



};