#pragma once
#include "SSEngineDefault/SSNativeTypes.h"

#include <d3d11.h>
#include <Windows.h>
#include <fbxsdk.h>

#include "SSAssetBase.h"

struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};

struct PhongVertex {
	Vector4f Pos;
	Vector4f Normal;
	Vector4f Tangent;
	Vector2f Uv1;
	Vector2f Uv2;
};


enum class GeometryDrawTopology {
	NONE = 0,
	TRIANGLELIST = 1,
	POINTLIST = 2,
};


class SSGeometryAsset : public SSAssetBase
{
	friend class SSFBXImporter;
private:
	GeometryDrawTopology _drawTopologyType = GeometryDrawTopology::NONE;

	PhongVertex* _vertexData = nullptr;
	uint32 _vertexDataSize = 0; // == _eachVertexDataSize * _vertexDataNum
	uint32 _eachVertexDataSize = 0;
	uint32 _vertexDataNum = 0;
	ID3D11Buffer* _vertexBuffer = nullptr;

	void* _indexData = nullptr;
	uint32 _indexDataSize = 0; // == EachIndexSize * _indexDataNum
	uint32 _eachIndexDataSize = 0;
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


	void SetDrawTopology(GeometryDrawTopology InTopology) { _drawTopologyType = InTopology; }
	void BindGeometry(ID3D11DeviceContext* InDeviceContext) const;


public:
	FORCEINLINE uint32 GetIndexDataNum() const { return _indexDataNum; }
	

};


static D3D_PRIMITIVE_TOPOLOGY ConvertToD3DTopology(GeometryDrawTopology InTopology) {
	switch (InTopology) {

	case GeometryDrawTopology::NONE:
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	case GeometryDrawTopology::TRIANGLELIST:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case GeometryDrawTopology::POINTLIST:
		return D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;


	default:
		assert(false);
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

