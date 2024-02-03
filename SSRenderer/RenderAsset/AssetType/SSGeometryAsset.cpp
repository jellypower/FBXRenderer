#include "SSGeometryAsset.h"

#include "SSEngineDefault/SSDebugLogger.h"

#include <memory.h>

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

SSGeometryAsset::SSGeometryAsset()
	: SSAssetBase(AssetType::Geometry)
{
}

void SSGeometryAsset::ReleaseSystemData()
{
	free(_vertexData);
	_vertexData = nullptr;

	free(_indexData);
	_indexData = nullptr;
}



HRESULT SSGeometryAsset::UpdateDataOnGPU(ID3D11Device* InDevice)
{
	if (_vertexData == nullptr) {
		SS_CLASS_WARNING_LOG("Vertex data must be initialized");
		return E_FAIL;
	}

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = _eachVertexDataSize * _vertexDataNum;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {}; // 버퍼의 초기 데이터는 여기 써 넣어준다.
	InitData.pSysMem = _vertexData;
	HRESULT hr = InDevice->CreateBuffer(&bd, &InitData, &_vertexBuffer);

	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG(" Vertex buffer creation failed.");
		return hr;
	}

	SSGeometryAsset* myPtr = nullptr;

	if (_indexData == nullptr) {
		SS_CLASS_ERR_LOG(" Index data must be initialzed");
		return E_FAIL;
	}

	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(uint32) * _wholeIndexDataNum;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	InitData = {}; // 버퍼의 초기 데이터는 여기 써 넣어준다.
	InitData.pSysMem = _indexData;
	hr = InDevice->CreateBuffer(&bd, &InitData, &_indexBuffer);

	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG(" Index buffer creation failed.");
		return hr;
	}

	return hr;
}

void SSGeometryAsset::ReleaseGPUData()
{
	if (_vertexBuffer == nullptr) return;

	_vertexBuffer->Release();
	_vertexBuffer = nullptr;
	_eachVertexDataSize = 0;

	if (_indexBuffer == nullptr) return;
	_indexBuffer->Release();
}

void SSGeometryAsset::BindGeometry(ID3D11DeviceContext* InDeviceContext, uint32 subGeomIdx) const
{
	uint32 offset = 0;
	InDeviceContext->IASetVertexBuffers(0, 1, &_vertexBuffer, &_eachVertexDataSize, &offset);
	InDeviceContext->IASetIndexBuffer(_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	InDeviceContext->IASetPrimitiveTopology(ConvertToD3DTopology(_drawTopologyType));
}
