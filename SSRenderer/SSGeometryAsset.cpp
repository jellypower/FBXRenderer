#include "SSGeometryAsset.h"

#include "SSEngineDefault/SSDebugLogger.h"

#include <memory.h>



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

	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG(" Vertex buffer creation failed.");
		return hr;
	}

	return hr;
}

void SSGeometryAsset::ReleaseVertexDataOnGPU()
{
	if (VertexBuffer == nullptr) return;

	VertexBuffer->Release();
	VertexBuffer = nullptr;

	EachVertexDataSize = 0;
	VertexDataSize = 0;

}

void SSGeometryAsset::LoadIndexDataOnSystemTemp()
{
	if (IndexData != nullptr) {
		SS_CLASS_WARNING_LOG("memory leakage possible.");
	}

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
		IndexDataNum = 36;
		IndexData = DBG_MALLOC(IndexDataSize);

		memcpy_s(IndexData, IndexDataSize, indices, IndexDataSize);

	}


}

void SSGeometryAsset::ReleaseIndexDataOnSystem()
{
	free(IndexData);
	IndexData = nullptr;

}

HRESULT SSGeometryAsset::SendIndexDataOnGPU(ID3D11Device* InDevice)
{
	if (IndexData == nullptr) {
		SS_CLASS_ERR_LOG(" Index data must be initialzed");
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


	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG(" Index buffer creation failed.");
		return hr;
	}

	return hr;
}

void SSGeometryAsset::ReleaseIndexDataOnGPU()
{
	if (IndexBuffer == nullptr) return;
	IndexBuffer->Release();
	IndexBuffer = nullptr;
	IndexDataSize = 0;
}

HRESULT SSGeometryAsset::InitGeometryDataOnSystem(fbxsdk::FbxMesh* InFbxMesh)
{
	if (VertexData != nullptr) {
		SS_CLASS_WARNING_LOG("memory leakage possible. function just return.");
		return E_FAIL;
	}


	// 1. Load num
	const int layerNum = InFbxMesh->GetLayerCount();
	const int ControlPointNum = InFbxMesh->GetControlPointsCount();
	const int PolygonNum = InFbxMesh->GetPolygonCount();
	const int PolygonVertexNum = InFbxMesh->GetPolygonVertexCount(); // sum of vertex in each polygon
	const FbxGeometryElementNormal* const FbxNormal = InFbxMesh->GetElementNormal();

	assert(FbxNormal != nullptr);

	// 2. alloc vertex memory
	switch (FbxNormal->GetMappingMode()) {

	case FbxLayerElement::eByControlPoint:
		VertexDataNum = ControlPointNum; break;
	case FbxLayerElement::eByPolygonVertex:
		VertexDataNum = PolygonVertexNum; break;

	default:
		free(IndexData);
		SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
		return E_FAIL;
	}

	EachVertexDataSize = sizeof(PhongVertex);
	VertexDataSize = EachVertexDataSize * VertexDataNum;
	VertexData = DBG_MALLOC(VertexDataSize);
	PhongVertex* phVertices = (PhongVertex*)VertexData;

	// 3. alloc index memory
	constexpr int VERTEX_NUM_IN_A_TRIANGLE = 3;
	int TriangleNumInModel = 0;
	for (int i = 0; i < InFbxMesh->GetPolygonCount(); i++) {
		TriangleNumInModel += (InFbxMesh->GetPolygonSize(i) - 2);
	}

	EachIndexDataSize = sizeof(uint16);
	IndexDataNum = TriangleNumInModel * VERTEX_NUM_IN_A_TRIANGLE;
	IndexDataSize = EachIndexDataSize * IndexDataNum;
	IndexData = DBG_MALLOC(IndexDataSize);

	uint16* IndexData16 = (uint16*)IndexData;
	uint32 CurIndexDataIdx = 0;


	// 4. Load vertex/Index data
	FbxVector4* ControlPoints = InFbxMesh->GetControlPoints();
//	int VertexIndexCounter = 0;
	int VertexIdxCounter = 0;

	switch (FbxNormal->GetMappingMode()) {

	case FbxLayerElement::eByControlPoint:

		for (int32 i = 0; i < InFbxMesh->GetControlPointsCount(); i++) {
			phVertices[i].Pos.X = ControlPoints[i].mData[0];
			phVertices[i].Pos.Y = ControlPoints[i].mData[1];
			phVertices[i].Pos.Z = ControlPoints[i].mData[2];
			phVertices[i].Pos.W = ControlPoints[i].mData[3];
		}

		for (uint32 i = 0; i < InFbxMesh->GetPolygonCount(); i++) {
			for (uint32 j = 1; j < InFbxMesh->GetPolygonSize(i) - 1; j++) {
				// Vertex를 fbx의 control point 그대로 저장하고 있음.
				// 내가 중복저장한 Vertex의 위치와 원본의 Vertex가 같음
				IndexData16[CurIndexDataIdx++] = InFbxMesh->GetPolygonVertex(i, 0);
				IndexData16[CurIndexDataIdx++] = InFbxMesh->GetPolygonVertex(i, j );
				IndexData16[CurIndexDataIdx++] = InFbxMesh->GetPolygonVertex(i, j + 1);
			}
		}
		assert(IndexDataNum == CurIndexDataIdx);

		break;
	case FbxLayerElement::eByPolygonVertex:

		for (int32 i = 0; i < InFbxMesh->GetPolygonCount(); i++) {
			for (int32 j = 0; j < InFbxMesh->GetPolygonSize(i); j++) {
				int32 ControlPointIdx = InFbxMesh->GetPolygonVertex(i, j);

				phVertices[VertexIdxCounter].Pos.X = ControlPoints[ControlPointIdx].mData[0];
				phVertices[VertexIdxCounter].Pos.Y = ControlPoints[ControlPointIdx].mData[1];
				phVertices[VertexIdxCounter].Pos.Z = ControlPoints[ControlPointIdx].mData[2];
				phVertices[VertexIdxCounter].Pos.W = ControlPoints[ControlPointIdx].mData[3];
				VertexIdxCounter++;

			}
		}
		assert(VertexIdxCounter == PolygonVertexNum);
		VertexIdxCounter = 0;

		for (uint32 i = 0; i < InFbxMesh->GetPolygonCount(); i++) {
			for (uint32 j = 1; j < InFbxMesh->GetPolygonSize(i) - 1; j++) {
				// Vertex를 중복저장해야 하고 있음. 
				// 내가 중복저장한 Vertex의 위치와 원본의 Vertex의 위치가 다름
				IndexData16[CurIndexDataIdx++] = VertexIdxCounter;
				IndexData16[CurIndexDataIdx++] = VertexIdxCounter + j;
				IndexData16[CurIndexDataIdx++] = VertexIdxCounter + j + 1;


			}
			VertexIdxCounter += InFbxMesh->GetPolygonSize(i);
		}
		assert(IndexDataNum == CurIndexDataIdx);

		break;
	default:
		free(IndexData);
		free(VertexData);
		SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
		return E_FAIL;
	}

	// 5. Load normal data
	switch (FbxNormal->GetMappingMode()) {
	case FbxLayerElement::EMappingMode::eByControlPoint:


		switch (FbxNormal->GetReferenceMode()) {
		case FbxLayerElement::EReferenceMode::eDirect:

			for (int32 i = 0; i < InFbxMesh->GetControlPointsCount(); i++) {
				phVertices[i].Normal.X = FbxNormal->GetDirectArray().GetAt(i).mData[0];
				phVertices[i].Normal.Y = FbxNormal->GetDirectArray().GetAt(i).mData[1];
				phVertices[i].Normal.Z = FbxNormal->GetDirectArray().GetAt(i).mData[2];
				phVertices[i].Normal.W = FbxNormal->GetDirectArray().GetAt(i).mData[3];
			}

			break;
		case FbxLayerElement::EReferenceMode::eIndex:
			SS_CLASS_WARNING_LOG("\"FbxLayerElement::EReferenceMode::eIndex\" is Legacy. Use \"eIndexToDirect\".");
		case FbxLayerElement::EReferenceMode::eIndexToDirect:

			for (int32 i = 0; i < InFbxMesh->GetControlPointsCount(); i++) {
				int32 FbxNormalIdx = FbxNormal->GetIndexArray().GetAt(i);

				phVertices[i].Normal.X = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[0];
				phVertices[i].Normal.Y = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[1];
				phVertices[i].Normal.Z = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[2];
				phVertices[i].Normal.W = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[3];
			}

			break;
		default:
			free(IndexData);
			free(VertexData);
			SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
			return E_FAIL;
		}

		break;
	case FbxLayerElement::EMappingMode::eByPolygonVertex:

		VertexIdxCounter = 0;

		switch (FbxNormal->GetReferenceMode()) {
		case FbxLayerElement::EReferenceMode::eDirect:

			// NOTE: GetPolygonVertex함수는 실제 IndexArray에 들어가서 그리는 것을 위한 Index를 반환해 주는 것이다.
			// 그러니까 이놈으로 노말 정보를 매핑하면 원하는 정보가 나오지 않는 것은 당연하다.
			// Index버퍼 기준으로 VertexBuffer를 샘플링 하는데 순서가 바르게 나올리가 없지

			// 심지어 위에서 Vertex Index 버퍼 로딩할 때 InFbxMesh->GetPolygonVertex(i, j); 로 로딩해서
			// 이미 정점 순서는 맞춰져있잖아? 그러니까 이미 맞춰준 순서인데 다시 GetPolygonVertex(i, j); 해줄 필요가 없지

			// input layout이 잘못 지정돼서 한 칸 씩 밀려 들어가는게 문제인 것으로 보인다. -> 가령 Normal의 W값이 Color의 X값으로 밀려간다거나 하는 상황.
			// 이제 해결하기 쉽겠지?

			for (uint32 i = 0; i < InFbxMesh->GetPolygonCount(); i++) {
				for (uint32 j = 0; j < InFbxMesh->GetPolygonSize(i); j++) {
					int32 ControlPointIdx = InFbxMesh->GetPolygonVertex(i, j);
					
					phVertices[VertexIdxCounter].Normal.X = FbxNormal->GetDirectArray().GetAt(VertexIdxCounter).mData[0];
					phVertices[VertexIdxCounter].Normal.Y = FbxNormal->GetDirectArray().GetAt(VertexIdxCounter).mData[1];
					phVertices[VertexIdxCounter].Normal.Z = FbxNormal->GetDirectArray().GetAt(VertexIdxCounter).mData[2];
					phVertices[VertexIdxCounter].Normal.W = FbxNormal->GetDirectArray().GetAt(VertexIdxCounter).mData[3];
				
					VertexIdxCounter++;
				}
			}
			assert(VertexIdxCounter == VertexDataNum);

			break;
		case FbxLayerElement::EReferenceMode::eIndex:
			SS_CLASS_WARNING_LOG("\"FbxLayerElement::EReferenceMode::eIndex\" is Legacy. Use \"eIndexToDirect\".");
		case FbxLayerElement::EReferenceMode::eIndexToDirect:

			for (int32 i = 0; i < InFbxMesh->GetPolygonCount(); i++) {
				for (int32 j = 0; j < InFbxMesh->GetPolygonSize(i); j++) {
					int32 FbxNormalIdx = FbxNormal->GetIndexArray().GetAt(VertexIdxCounter);

					phVertices[VertexIdxCounter].Normal.X = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[0];
					phVertices[VertexIdxCounter].Normal.Y = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[1];
					phVertices[VertexIdxCounter].Normal.Z = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[2];
					phVertices[VertexIdxCounter].Normal.W = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[3];

					VertexIdxCounter++;
				}
			}
			assert(VertexIdxCounter == VertexDataNum);

			break;
		default:
			free(IndexData);
			free(VertexData);
			SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
			return E_FAIL;
		}

		break;
	default:
		free(IndexData);
		free(VertexData);
		SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
		return E_FAIL;
	}

	// 6. Load Color
	FbxGeometryElementVertexColor* FbxColor = InFbxMesh->GetElementVertexColor();
	if (FbxColor != nullptr) {
		switch (FbxColor->GetMappingMode()) {

		}
	}

	// TODO: 컬러 칠하기

	for (int i = 0; i < VertexDataNum; i++) {
		
		phVertices[i].Color = Vector4f(1, 1, 1, 1);
		phVertices[i].Color = Vector4f(1, 1, 1, 1);
		phVertices[i].Color = Vector4f(1, 1, 1, 1);
		phVertices[i].Color = Vector4f(1, 1, 1, 1);
	}


	return S_OK;
}

void SSGeometryAsset::BindGeometry(ID3D11DeviceContext* InDeviceContext)
{
	uint32 offset = 0;
	InDeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &EachVertexDataSize, &offset);
	InDeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	InDeviceContext->IASetPrimitiveTopology(ConvertToD3DTopology(DrawTopologyType));
}

