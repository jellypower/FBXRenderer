#include "SSGeometryAsset.h"

#include "SSEngineDefault/SSDebugLogger.h"

#include <memory.h>


SSGeometryAsset::SSGeometryAsset()
{
	_assetType = AssetType::Geometry;
}

void SSGeometryAsset::ReleaseVertexDataOnSystem()
{
	free(_vertexData);
	_vertexData = nullptr;
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

	D3D11_SUBRESOURCE_DATA InitData = {}; // 버퍼의 초기 데이터는 여기 써 넣어준다.
	InitData.pSysMem = _vertexData;
	HRESULT hr = InDevice->CreateBuffer(&bd, &InitData, &_vertexBuffer);

	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG(" Vertex buffer creation failed.");
		return hr;
	}

	return hr;
}

void SSGeometryAsset::ReleaseVertexDataOnGPU()
{
	if (_vertexBuffer == nullptr) return;

	_vertexBuffer->Release();
	_vertexBuffer = nullptr;

	_eachVertexDataSize = 0;
	_vertexDataSize = 0;

}

void SSGeometryAsset::ReleaseIndexDataOnSystem()
{
	free(_indexData);
	_indexData = nullptr;
}

HRESULT SSGeometryAsset::SendIndexDataOnGPU(ID3D11Device* InDevice)
{
	if (_indexData == nullptr) {
		SS_CLASS_ERR_LOG(" Index data must be initialzed");
		return E_FAIL;
	}

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = _indexDataSize;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {}; // 버퍼의 초기 데이터는 여기 써 넣어준다.
	InitData.pSysMem = _indexData;
	HRESULT hr = InDevice->CreateBuffer(&bd, &InitData, &_indexBuffer);


	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG(" Index buffer creation failed.");
		return hr;
	}

	return hr;
}

void SSGeometryAsset::ReleaseIndexDataOnGPU()
{
	if (_indexBuffer == nullptr) return;
	_indexBuffer->Release();
	_indexBuffer = nullptr;
	_indexDataSize = 0;
}

HRESULT SSGeometryAsset::InitGeometryDataOnSystem(fbxsdk::FbxMesh* InFbxMesh)
{
	if (_vertexData != nullptr) {
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
		_vertexDataNum = ControlPointNum; break;
	case FbxLayerElement::eByPolygonVertex:
		_vertexDataNum = PolygonVertexNum; break;

	default:
		free(_indexData);
		SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
		return E_FAIL;
	}

	_eachVertexDataSize = sizeof(PhongVertex);
	_vertexDataSize = _eachVertexDataSize * _vertexDataNum;
	_vertexData = (PhongVertex*)DBG_MALLOC(_vertexDataSize);
	PhongVertex* phVertices = (PhongVertex*)_vertexData;

	// 3. alloc index memory
	constexpr int VERTEX_NUM_IN_A_TRIANGLE = 3;
	int TriangleNumInModel = 0;
	for (int i = 0; i < InFbxMesh->GetPolygonCount(); i++) {
		TriangleNumInModel += (InFbxMesh->GetPolygonSize(i) - 2);
	}

	_eachIndexDataSize = sizeof(uint16);
	_indexDataNum = TriangleNumInModel * VERTEX_NUM_IN_A_TRIANGLE;
	_indexDataSize = _eachIndexDataSize * _indexDataNum;
	_indexData = DBG_MALLOC(_indexDataSize);

	uint16* IndexData16 = (uint16*)_indexData;
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
		assert(_indexDataNum == CurIndexDataIdx);

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
		assert(_indexDataNum == CurIndexDataIdx);

		break;
	default:
		free(_indexData);
		free(_vertexData);
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
			free(_indexData);
			free(_vertexData);
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
			assert(VertexIdxCounter == _vertexDataNum);

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
			assert(VertexIdxCounter == _vertexDataNum);

			break;
		default:
			free(_indexData);
			free(_vertexData);
			SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
			return E_FAIL;
		}

		break;
	default:
		free(_indexData);
		free(_vertexData);
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

	for (int i = 0; i < _vertexDataNum; i++) {
		
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
	InDeviceContext->IASetVertexBuffers(0, 1, &_vertexBuffer, &_eachVertexDataSize, &offset);
	InDeviceContext->IASetIndexBuffer(_indexBuffer, DXGI_FORMAT_R16_UINT, 0);

	InDeviceContext->IASetPrimitiveTopology(ConvertToD3DTopology(_drawTopologyType));
}

