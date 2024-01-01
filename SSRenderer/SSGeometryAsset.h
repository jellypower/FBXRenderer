#pragma once
#include "SSEngineDefault/SSNativeTypes.h"

#include <d3d11.h>
#include <Windows.h>
#include <fbxsdk.h>



enum class GeometryAssetInstanceStage {
	JustCreated = 0,
	Initialized = 1,
	Instantiated,
};



enum class GeometryDrawTopology {
	NONE = 0,
	TRIANGLELIST = 1
};


class SSGeometryAsset
{
private:
	GeometryDrawTopology DrawTopologyType = GeometryDrawTopology::NONE;

	void* VertexData = nullptr;
	uint32 VertexDataSize = 0; // == EachVertexDataSize * VertexDataNum
	uint32 EachVertexDataSize = 0;
	uint32 VertexDataNum = 0;
	ID3D11Buffer* VertexBuffer = nullptr;


	void* IndexData = nullptr;
	uint32 IndexDataSize = 0; // == EachIndexSize * IndexDataNum
	uint32 EachIndexDataSize = 0;
	uint32 IndexDataNum = 0;
	ID3D11Buffer* IndexBuffer = nullptr;


public:

	void ReleaseVertexDataOnSystem();
	bool DoesExistVertexDataOnSystem() { return VertexData != nullptr; }

	HRESULT SendVertexDataOnGPU(ID3D11Device* InDevice);
	void ReleaseVertexDataOnGPU();

	// HACK:
	void LoadIndexDataOnSystemTemp();

	void ReleaseIndexDataOnSystem();
	bool DoesExistIndexDataOnSystem() { return IndexData != nullptr; }

	HRESULT SendIndexDataOnGPU(ID3D11Device* InDevice);
	void ReleaseIndexDataOnGPU();

	HRESULT InitGeometryDataOnSystem(FbxMesh* InFbxMesh);

	void SetDrawTopology(GeometryDrawTopology InTopology) { DrawTopologyType = InTopology; }
	void BindGeometry(ID3D11DeviceContext* InDeviceContext);
	inline bool IsBindingPossible();

public:
	__forceinline uint32 GetIndexDataNum() { return IndexDataNum; }
	

};


static D3D_PRIMITIVE_TOPOLOGY ConvertToD3DTopology(GeometryDrawTopology InTopology) {
	switch (InTopology) {

	case GeometryDrawTopology::NONE:
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	case GeometryDrawTopology::TRIANGLELIST:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


	default:
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

inline bool SSGeometryAsset::IsBindingPossible()
{
	return (
		VertexData != nullptr && IndexData != nullptr
		);
}

