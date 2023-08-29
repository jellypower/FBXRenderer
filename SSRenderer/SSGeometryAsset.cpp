#include "SSGeometryAsset.h"

#include "SSDebug.h"

#include <memory.h>

// HACK: 나중에는 파일, 혹은 다른 지오메트리로부터 생성하기

// HACK: SimpleVertex 나중에 없애기
struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};


void SSGeometryAsset::InitVertexDataOnSystem()
{
	// HACK:
	{
        SimpleVertex vertices[] =
        {
            { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
            { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
            { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
            { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },

            { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
            { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
            { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
            { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

            { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
            { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
            { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
            { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },

            { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
            { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
            { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
            { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },

            { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
            { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
            { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
            { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },

            { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
            { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
            { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
            { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
        };

        VertexDataSize = sizeof(SimpleVertex) * 24;
        EachVertexSize = sizeof(SimpleVertex);
        VertexData = malloc(VertexDataSize);

        memcpy_s(VertexData, VertexDataSize, vertices, VertexDataSize);

	}
    

}

void SSGeometryAsset::ReleaseVertexDataOnSystem()
{
    free(VertexData);
    VertexData = nullptr;
}

HRESULT SSGeometryAsset::SendVertexDataOnGPU(ID3D11Device* InDevice)
{
    if (VertexData == nullptr) {
        SS_LOG("Error(SSGeometryAsset::SendVertexDataOnGPU): Vertex data must be initialzed");
        return E_FAIL;
    }

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = VertexDataSize;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData = {}; // 버퍼의 초기 데이터는 여기 써 넣어준다.
    InitData.pSysMem = VertexData;
    HRESULT hr = InDevice->CreateBuffer(&bd, &InitData, &VertexBuffer);

    InDevice->SetPrivateData(WKPDID_D3DDebugObjectName, 6,"Vertex");

    if (FAILED(hr)) {
        SS_LOG("Error(SSGeometryAsset::SendVertexDataOnGPU): Vertex buffer creation failed.");
        return hr;
    }

    return hr;
}

void SSGeometryAsset::ReleaseVertexDataOnGPU()
{
    VertexBuffer->Release();
    VertexBuffer = nullptr;

    EachVertexSize = 0;
    VertexDataSize = 0;

}

void SSGeometryAsset::LoadIndexDataOnSystem()
{

    // HACK:
    {
        WORD indices[] =
        {
            3,1,0,
            2,1,3,

            6,4,5,
            7,4,6,

            11,9,8,
            10,9,11,

            14,12,13,
            15,12,14,

            19,17,16,
            18,17,19,

            22,20,21,
            23,20,22
        };


        IndexDataSize = sizeof(WORD) * 36;
        IndexDataCount = 36;
        IndexData = malloc(IndexDataSize);

        memcpy_s(IndexData, IndexDataSize, indices, IndexDataSize);

    }

    DrawMethologyType = GeometryDrawMethology::DRAW_INDEX;

}

void SSGeometryAsset::ReleaseIndexDataOnSystem()
{
    free(IndexData);
    IndexData = nullptr;
    
}

HRESULT SSGeometryAsset::SendIndexDataOnGPU(ID3D11Device* InDevice)
{
    if (IndexData == nullptr) {
        SS_LOG("Error(SSGeometryAsset::SendIndexDataOnGPU): Index data must be initialzed");
        return E_FAIL;
    }

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = IndexDataSize;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData = {}; // 버퍼의 초기 데이터는 여기 써 넣어준다.
    InitData.pSysMem = IndexData;
    HRESULT hr = InDevice->CreateBuffer(&bd, &InitData, &IndexBuffer);

    InDevice->SetPrivateData(WKPDID_D3DDebugObjectName, 5, "Index");


    if (FAILED(hr)) {
        SS_LOG("Error(SSGeometryAsset::SendIndexDataOnGPU): index buffer creation failed.");
        return hr;
    }

    return hr;
}

void SSGeometryAsset::ReleaseIndexDataOnGPU()
{
    IndexBuffer->Release();
    IndexBuffer = nullptr;
    IndexDataSize = 0;
}

void SSGeometryAsset::BindGeometry(ID3D11DeviceContext* InDeviceContext)
{
    uint32 offset = 0;
    InDeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &EachVertexSize, &offset);

    if (DrawMethologyType == GeometryDrawMethology::DRAW_INDEX) {
        InDeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    }

    InDeviceContext->IASetPrimitiveTopology(ConvertToD3DTopology(DrawTopologyType));
}

