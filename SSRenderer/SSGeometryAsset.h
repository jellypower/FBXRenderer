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
	Vector4f Color;
};


enum class GeometryDrawTopology {
	NONE = 0,
	TRIANGLELIST = 1
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
	virtual ~SSGeometryAsset() override { };

	void ReleaseVertexDataOnSystem();
	bool UsableOnSystem() { return _vertexData != nullptr; }

	HRESULT UpdateDataOnGPU(ID3D11Device* InDevice);
	void ReleaseVertexDataOnGPU();

	void ReleaseIndexDataOnSystem();
	bool DoesExistIndexDataOnSystem() { return _indexData != nullptr; }

	HRESULT SendIndexDataOnGPU(ID3D11Device* InDevice);
	void ReleaseIndexDataOnGPU();

	HRESULT InitGeometryDataOnSystem(FbxMesh* InFbxMesh);

	void SetDrawTopology(GeometryDrawTopology InTopology) { _drawTopologyType = InTopology; }
	void BindGeometry(ID3D11DeviceContext* InDeviceContext);
	inline bool IsBindingPossible();

public:
	FORCEINLINE uint32 GetIndexDataNum() { return _indexDataNum; }
	

};


static D3D_PRIMITIVE_TOPOLOGY ConvertToD3DTopology(GeometryDrawTopology InTopology) {
	switch (InTopology) {

	case GeometryDrawTopology::NONE:
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	case GeometryDrawTopology::TRIANGLELIST:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


	default:
		assert(false);
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

inline bool SSGeometryAsset::IsBindingPossible()
{
	return (
		_vertexData != nullptr && _indexData != nullptr
		);
}

