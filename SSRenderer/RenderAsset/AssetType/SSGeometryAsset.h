#pragma once
#include "SSEngineDefault/SSNativeTypes.h"

#include <d3d11.h>
#include <Windows.h>

#include "SSAssetBase.h"


constexpr uint32 MULTIMATERIAL_COUNT_MAX = 8;

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
	uint32 _vertexDataSize = 0; // == _eachVertexDataSize * _vertexDataNum
	uint32 _eachVertexDataSize = 0;
	uint32 _vertexDataNum = 0;
	ID3D11Buffer* _vertexBuffer = nullptr;


	uint32* _indexData;
	uint32 _indexDataNum = 0;
	ID3D11Buffer* _indexBuffer = nullptr;


public:
	SSGeometryAsset();
	virtual ~SSGeometryAsset() override = default;

	void ReleaseSystemData();
	bool UsableOnSystem() const { return _vertexData != nullptr; }

	HRESULT UpdateDataOnGPU(ID3D11Device* InDevice);
	bool UsableOnGPU() const { return _vertexBuffer != nullptr; }
	void ReleaseGPUData();

	bool DoesExistIndexDataOnSystem() const { return _indexData != nullptr; }


	void SetDrawTopology(EGeometryDrawTopology InTopology) { _drawTopologyType = InTopology; }
	void BindGeometry(ID3D11DeviceContext* InDeviceContext) const;


public:
	FORCEINLINE uint32 GetIndexDataNum() const { return _indexDataNum; }
	

};


static D3D_PRIMITIVE_TOPOLOGY ConvertToD3DTopology(EGeometryDrawTopology InTopology) {
	switch (InTopology) {

	case EGeometryDrawTopology::None:
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	case EGeometryDrawTopology::TriangleList:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case EGeometryDrawTopology::PointList:
		return D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;


	default:
		assert(false);
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

