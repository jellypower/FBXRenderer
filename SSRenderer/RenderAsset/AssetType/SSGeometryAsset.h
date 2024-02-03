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

struct SSDefaultVertex {
	Vector4f Pos;
	Vector4f Normal;
	Vector4f Tangent;
	Vector2f Uv[2];
	Vector4f WorldPos;
};


enum class EGeometryDrawTopology {
	None = 0,
	TriangleList = 1,
	PointList = 2,
};

enum class EVertexUnit
{
	None,

	VertexPerPoint, // 같은 position에 있는 놈들은 전부 1개의 정점을 공유
	VertexPerPolygon, // 하나의 polygon(다각형)끼리 정점을 공유
};


class SSGeometryAsset : public SSAssetBase
{
	friend class SSFBXImporter;
private:
	EGeometryDrawTopology _drawTopologyType = EGeometryDrawTopology::None;
	EVertexUnit _vertexUnit = EVertexUnit::None;

	SSDefaultVertex* _vertexData = nullptr;
	uint32 _eachVertexDataSize = 0;
	uint32 _vertexDataNum = 0;
	ID3D11Buffer* _vertexBuffer = nullptr;


	uint8 _subGeometryNum;
	uint32* _indexData = nullptr;
	uint32 _indexDataNum[SUBGEOM_COUNT_MAX] = {0, };
	uint32 _indexDataStartIndex[SUBGEOM_COUNT_MAX] = { 0, };
	uint32 _wholeIndexDataNum = 0;
	ID3D11Buffer* _indexBuffer = nullptr;


public:
	SSGeometryAsset();
	virtual ~SSGeometryAsset() override = default;

	void ReleaseSystemData();
	bool UsableOnSystem() const { return _vertexData != nullptr; }

	HRESULT UpdateDataOnGPU(ID3D11Device* InDevice);
	bool UsableOnGPU() const { return _vertexBuffer != nullptr; }
	void ReleaseGPUData();

	void SetDrawTopology(EGeometryDrawTopology InTopology) { _drawTopologyType = InTopology; }
	void BindGeometry(ID3D11DeviceContext* InDeviceContex, uint32 subGeomIdx = 0) const;

public:
	FORCEINLINE uint8 GetSubGeometryNum() const { return _subGeometryNum; }
	FORCEINLINE uint32 GetIndexDataStartIndex(uint32 subGeomIdx = 0) const { return _indexDataStartIndex[subGeomIdx]; }
	FORCEINLINE uint32 GetIndexDataNum(uint32 subGeomIdx = 0) const { return _indexDataNum[subGeomIdx]; }
	

};