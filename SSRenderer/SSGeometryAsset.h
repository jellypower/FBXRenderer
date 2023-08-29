#pragma once
#include "SSNativeTypes.h"

#include <d3d11.h>
#include <Windows.h>

enum class GeometryDrawMethology {
	DRAW_VERTEX = 0,
	DRAW_INDEX = 1
};

enum class GeometryDrawTopology {
	NONE = 0,
	TRIANGLELIST = 1
};

class SSGeometryAsset
{
	// TODO: 셰이더에 추가하고 나서 여기에도
	// TODO:  Layout Info 추가 또는 Layout Info 규격화하기 (둘 다 할수도 있고)

public:
	void InitVertexDataOnSystem(/** TODO: from file or from other geometry*/);
	void ReleaseVertexDataOnSystem();
	bool IsVertexDataOnSystem() { return VertexData != nullptr; }

	HRESULT SendVertexDataOnGPU(ID3D11Device* InDevice);
	void ReleaseVertexDataOnGPU();
	bool IsVertexDataOnGPU() { return VertexBuffer != nullptr; }

	void LoadIndexDataOnSystem();
	void ReleaseIndexDataOnSystem();
	bool IsIndexDataOnSystem() { return IndexData != nullptr; }

	HRESULT SendIndexDataOnGPU(ID3D11Device* InDevice);
	void ReleaseIndexDataOnGPU();
	bool IsIndexDataOnGPU() { return IndexBuffer != nullptr; }

	void SetDrawTopology(GeometryDrawTopology InTopology) { DrawTopologyType = InTopology; }
	void BindGeometry(ID3D11DeviceContext* InDeviceContext);
	inline bool IsBindingPossible();

public:
	__forceinline uint32 GetIndexDataCount() { return IndexDataCount; }
	
private:


	GeometryDrawMethology DrawMethologyType = GeometryDrawMethology::DRAW_VERTEX;
	GeometryDrawTopology DrawTopologyType = GeometryDrawTopology::NONE;

	void* VertexData = nullptr;
	uint32 VertexDataSize = 0;
	uint32 EachVertexSize = 0;

	ID3D11Buffer* VertexBuffer = nullptr;
	

	void* IndexData = nullptr;
	uint32 IndexDataSize = 0;
	uint32 IndexDataCount = 0;

	ID3D11Buffer* IndexBuffer = nullptr;
	
};


static D3D_PRIMITIVE_TOPOLOGY ConvertToD3DTopology(GeometryDrawTopology InTopology) {
	switch (InTopology) {

	case GeometryDrawTopology::NONE:
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	case GeometryDrawTopology::TRIANGLELIST:
		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


	default:
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

inline bool SSGeometryAsset::IsBindingPossible()
{
	return IsVertexDataOnGPU()&&
		(
			(DrawMethologyType == GeometryDrawMethology::DRAW_INDEX && IsIndexDataOnGPU())
			||
			DrawMethologyType == GeometryDrawMethology::DRAW_VERTEX
			)

		;
}

