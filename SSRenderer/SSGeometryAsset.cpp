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

	D3D11_SUBRESOURCE_DATA InitData = {}; // ������ �ʱ� �����ʹ� ���� �� �־��ش�.
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

	D3D11_SUBRESOURCE_DATA InitData = {}; // ������ �ʱ� �����ʹ� ���� �� �־��ش�.
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
				// Vertex�� fbx�� control point �״�� �����ϰ� ����.
				// ���� �ߺ������� Vertex�� ��ġ�� ������ Vertex�� ����
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
				// Vertex�� �ߺ������ؾ� �ϰ� ����. 
				// ���� �ߺ������� Vertex�� ��ġ�� ������ Vertex�� ��ġ�� �ٸ�
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

			// NOTE: GetPolygonVertex�Լ��� ���� IndexArray�� ���� �׸��� ���� ���� Index�� ��ȯ�� �ִ� ���̴�.
			// �׷��ϱ� �̳����� �븻 ������ �����ϸ� ���ϴ� ������ ������ �ʴ� ���� �翬�ϴ�.
			// Index���� �������� VertexBuffer�� ���ø� �ϴµ� ������ �ٸ��� ���ø��� ����

			// ������ ������ Vertex Index ���� �ε��� �� InFbxMesh->GetPolygonVertex(i, j); �� �ε��ؼ�
			// �̹� ���� ������ ���������ݾ�? �׷��ϱ� �̹� ������ �����ε� �ٽ� GetPolygonVertex(i, j); ���� �ʿ䰡 ����

			// input layout�� �߸� �����ż� �� ĭ �� �з� ���°� ������ ������ ���δ�. -> ���� Normal�� W���� Color�� X������ �з����ٰų� �ϴ� ��Ȳ.
			// ���� �ذ��ϱ� ������?

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

	// TODO: �÷� ĥ�ϱ�

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

