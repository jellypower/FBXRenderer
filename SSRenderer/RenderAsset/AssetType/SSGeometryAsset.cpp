#include "SSGeometryAsset.h"

#include "SSEngineDefault/SSDebugLogger.h"

#include <memory.h>


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
	bd.ByteWidth = _vertexDataSize;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {}; // ������ �ʱ� �����ʹ� ���� �� �־��ش�.
	InitData.pSysMem = _vertexData;
	HRESULT hr = InDevice->CreateBuffer(&bd, &InitData, &_vertexBuffer);

	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG(" Vertex buffer creation failed.");
		return hr;
	}


	if (_indexData == nullptr) {
		SS_CLASS_ERR_LOG(" Index data must be initialzed");
		return E_FAIL;
	}

	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = _indexDataSize;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	InitData = {}; // ������ �ʱ� �����ʹ� ���� �� �־��ش�.
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
	_vertexDataSize = 0;

	if (_indexBuffer == nullptr) return;
	_indexBuffer->Release();
	_indexBuffer = nullptr;
	_indexDataSize = 0;
}

void SSGeometryAsset::BindGeometry(ID3D11DeviceContext* InDeviceContext) const
{
	uint32 offset = 0;
	InDeviceContext->IASetVertexBuffers(0, 1, &_vertexBuffer, &_eachVertexDataSize, &offset);
	InDeviceContext->IASetIndexBuffer(_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	InDeviceContext->IASetPrimitiveTopology(ConvertToD3DTopology(_drawTopologyType));
}
