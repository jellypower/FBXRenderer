
#include "SSFBXImporter.h"
#include "SSEngineDefault/SSDebugLogger.h"
#include "SSRenderer/SSMaterialAssetManager.h"
#include "SSRenderer/SSGeometryAssetManager.h"




SSFBXImporter::SSFBXImporter()
{
	_FBXManager = ::FbxManager::Create();
	_IOSetting = FbxIOSettings::Create(_FBXManager, IOSROOT);
	_FBXImporter = fbxsdk::FbxImporter::Create(_FBXManager, "");

	_IOSetting->SetBoolProp(IMP_FBX_MATERIAL, true);
	_FBXManager->SetIOSettings(_IOSetting);

}

SSFBXImporter::~SSFBXImporter()
{
	_FBXImporter->Destroy();
	_IOSetting->Destroy();
	_FBXManager->Destroy();
}

HRESULT SSFBXImporter::LoadModelAssetFromFBXFile(const char* filePath)
{

	if (!_FBXImporter->Initialize(filePath, -1, _FBXManager->GetIOSettings())) {
		SS_CLASS_WARNING_LOG("%s", _FBXImporter->GetStatus().GetErrorString());
		return E_FAIL;
	}

	_currentScene = FbxScene::Create(_FBXManager, filePath);
	_FBXImporter->Import(_currentScene);
	_filePath = filePath;

	return S_OK;

}

void SSFBXImporter::ClearFBXModelAsset()
{
	_currentScene->Destroy();
	_currentScene = nullptr;
	_filePath.Clear();
}

void SSFBXImporter::StoreCurrentFBXModelAssetToAssetManager()
{
	TraverseNodes();
}


void SSFBXImporter::TraverseNodes()
{
	if (_currentScene == nullptr) {
		SS_CLASS_WARNING_LOG("No scene to load");
		return;
	}

	FbxNode* rootNode = _currentScene->GetRootNode();

	TraverseNodesRecursion(rootNode);
	
}

void SSFBXImporter::TraverseNodesRecursion(fbxsdk::FbxNode* node)
{
	if (node->GetNodeAttribute() != nullptr) {
		switch (node->GetNodeAttribute()->GetAttributeType())
		{	
		case FbxNodeAttribute::eMesh:
			fbxsdk::FbxMesh* Mesh = node->GetMesh();

			StoreModelToManager(Mesh);
			
			SS_LOG("\t%d\n", Mesh->GetLayerCount());
		
		}
	}

	for (int i = 0; i < node->GetChildCount(); i++) {
		TraverseNodesRecursion(node->GetChild(i));
	}
}

void SSFBXImporter::StoreModelToManager(fbxsdk::FbxMesh* InFBXMesh)
{
	SSGeometryAssetManager::Get()->InstantiateNewGeometry(InFBXMesh);
	
}

SSGeometryAsset* SSFBXImporter::GenerateGeometryFromFbxMesh(fbxsdk::FbxMesh* fbxMesh)
{
	assert(fbxMesh != nullptr);
	SSGeometryAsset* NewGeometryAsset = DBG_NEW SSGeometryAsset();

	// 1. Load num
	const int layerNum = fbxMesh->GetLayerCount();
	const int ControlPointNum = fbxMesh->GetControlPointsCount();
	const int PolygonNum = fbxMesh->GetPolygonCount();
	const int PolygonVertexNum = fbxMesh->GetPolygonVertexCount(); // sum of vertex in each polygon
	const FbxGeometryElementNormal* const FbxNormal = fbxMesh->GetElementNormal();

	assert(FbxNormal != nullptr);

	// 2. alloc vertex memory
	switch (FbxNormal->GetMappingMode()) {

	case FbxLayerElement::eByControlPoint:
		NewGeometryAsset->_vertexDataNum = ControlPointNum; break;
	case FbxLayerElement::eByPolygonVertex:
		NewGeometryAsset->_vertexDataNum = PolygonVertexNum; break;

	default:
		assert(false);
		SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
		return nullptr;
	}

	NewGeometryAsset->_eachVertexDataSize = sizeof(PhongVertex);
	NewGeometryAsset->_vertexDataSize = NewGeometryAsset->_eachVertexDataSize * NewGeometryAsset->_vertexDataNum;
	NewGeometryAsset->_vertexData = (PhongVertex*)DBG_MALLOC(NewGeometryAsset->_vertexDataSize);
	PhongVertex* phVertices = (PhongVertex*)NewGeometryAsset->_vertexData;

	// 3. alloc index memory
	constexpr int VERTEX_NUM_IN_A_TRIANGLE = 3;
	int TriangleNumInModel = 0;
	for (int i = 0; i < fbxMesh->GetPolygonCount(); i++) {
		TriangleNumInModel += (fbxMesh->GetPolygonSize(i) - 2);
	}

	NewGeometryAsset->_eachIndexDataSize = sizeof(uint16);
	NewGeometryAsset->_indexDataNum = TriangleNumInModel * VERTEX_NUM_IN_A_TRIANGLE;
	NewGeometryAsset->_indexDataSize = NewGeometryAsset->_eachIndexDataSize * NewGeometryAsset->_indexDataNum;
	NewGeometryAsset->_indexData = DBG_MALLOC(NewGeometryAsset->_indexDataSize);

	uint16* IndexData16 = (uint16*)NewGeometryAsset->_indexData;
	uint32 CurIndexDataIdx = 0;


	// 4. Load vertex/Index data
	FbxVector4* ControlPoints = fbxMesh->GetControlPoints();
	int VertexIdxCounter = 0;

	switch (FbxNormal->GetMappingMode()) {

	case FbxLayerElement::eByControlPoint:

		for (int32 i = 0; i < fbxMesh->GetControlPointsCount(); i++) {
			phVertices[i].Pos.X = ControlPoints[i].mData[0];
			phVertices[i].Pos.Y = ControlPoints[i].mData[1];
			phVertices[i].Pos.Z = ControlPoints[i].mData[2];
			phVertices[i].Pos.W = ControlPoints[i].mData[3];
		}

		for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++) {
			for (uint32 j = 1; j < fbxMesh->GetPolygonSize(i) - 1; j++) {
				// Vertex�� fbx�� control point �״�� �����ϰ� ����.
				// ���� �ߺ������� Vertex�� ��ġ�� ������ Vertex�� ����
				IndexData16[CurIndexDataIdx++] = fbxMesh->GetPolygonVertex(i, 0);
				IndexData16[CurIndexDataIdx++] = fbxMesh->GetPolygonVertex(i, j);
				IndexData16[CurIndexDataIdx++] = fbxMesh->GetPolygonVertex(i, j + 1);
			}
		}
		assert(NewGeometryAsset->_indexDataNum == CurIndexDataIdx);

		break;
	case FbxLayerElement::eByPolygonVertex:

		for (int32 i = 0; i < fbxMesh->GetPolygonCount(); i++) {
			for (int32 j = 0; j < fbxMesh->GetPolygonSize(i); j++) {
				int32 ControlPointIdx = fbxMesh->GetPolygonVertex(i, j);

				phVertices[VertexIdxCounter].Pos.X = ControlPoints[ControlPointIdx].mData[0];
				phVertices[VertexIdxCounter].Pos.Y = ControlPoints[ControlPointIdx].mData[1];
				phVertices[VertexIdxCounter].Pos.Z = ControlPoints[ControlPointIdx].mData[2];
				phVertices[VertexIdxCounter].Pos.W = ControlPoints[ControlPointIdx].mData[3];
				VertexIdxCounter++;

			}
		}
		assert(VertexIdxCounter == PolygonVertexNum);
		VertexIdxCounter = 0;

		for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++) {
			for (uint32 j = 1; j < fbxMesh->GetPolygonSize(i) - 1; j++) {
				// Vertex�� �ߺ������ؾ� �ϰ� ����. 
				// ���� �ߺ������� Vertex�� ��ġ�� ������ Vertex�� ��ġ�� �ٸ�
				IndexData16[CurIndexDataIdx++] = VertexIdxCounter;
				IndexData16[CurIndexDataIdx++] = VertexIdxCounter + j;
				IndexData16[CurIndexDataIdx++] = VertexIdxCounter + j + 1;


			}
			VertexIdxCounter += fbxMesh->GetPolygonSize(i);
		}
		assert(NewGeometryAsset->_indexDataNum == CurIndexDataIdx);

		break;
	default:
		free(NewGeometryAsset->_indexData);
		free(NewGeometryAsset->_vertexData);
		SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
		return nullptr;
	}

	// 5. Load normal data
	switch (FbxNormal->GetMappingMode()) {
	case FbxLayerElement::EMappingMode::eByControlPoint:


		switch (FbxNormal->GetReferenceMode()) {
		case FbxLayerElement::EReferenceMode::eDirect:

			for (int32 i = 0; i < fbxMesh->GetControlPointsCount(); i++) {
				phVertices[i].Normal.X = FbxNormal->GetDirectArray().GetAt(i).mData[0];
				phVertices[i].Normal.Y = FbxNormal->GetDirectArray().GetAt(i).mData[1];
				phVertices[i].Normal.Z = FbxNormal->GetDirectArray().GetAt(i).mData[2];
				phVertices[i].Normal.W = FbxNormal->GetDirectArray().GetAt(i).mData[3];
			}

			break;
		case FbxLayerElement::EReferenceMode::eIndex:
			SS_CLASS_WARNING_LOG("\"FbxLayerElement::EReferenceMode::eIndex\" is Legacy. Use \"eIndexToDirect\".");
		case FbxLayerElement::EReferenceMode::eIndexToDirect:

			for (int32 i = 0; i < fbxMesh->GetControlPointsCount(); i++) {
				int32 FbxNormalIdx = FbxNormal->GetIndexArray().GetAt(i);

				phVertices[i].Normal.X = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[0];
				phVertices[i].Normal.Y = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[1];
				phVertices[i].Normal.Z = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[2];
				phVertices[i].Normal.W = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[3];
			}

			break;
		default:
			free(NewGeometryAsset->_indexData);
			free(NewGeometryAsset->_vertexData);
			SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
			return nullptr;
		}

		break;
	case FbxLayerElement::EMappingMode::eByPolygonVertex:

		VertexIdxCounter = 0;

		switch (FbxNormal->GetReferenceMode()) {
		case FbxLayerElement::EReferenceMode::eDirect:

			// NOTE: GetPolygonVertex�Լ��� ���� IndexArray�� ���� �׸��� ���� ���� Index�� ��ȯ�� �ִ� ���̴�.
			// �׷��ϱ� �̳����� �븻 ������ �����ϸ� ���ϴ� ������ ������ �ʴ� ���� �翬�ϴ�.
			// Index���� �������� VertexBuffer�� ���ø� �ϴµ� ������ �ٸ��� ���ø��� ����

			// ������ ������ Vertex Index ���� �ε��� �� fbxMesh->GetPolygonVertex(i, j); �� �ε��ؼ�
			// �̹� ���� ������ ���������ݾ�? �׷��ϱ� �̹� ������ �����ε� �ٽ� GetPolygonVertex(i, j); ���� �ʿ䰡 ����

			// input layout�� �߸� �����ż� �� ĭ �� �з� ���°� ������ ������ ���δ�. -> ���� Normal�� W���� Color�� X������ �з����ٰų� �ϴ� ��Ȳ.
			// ���� �ذ��ϱ� ������?

			for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++) {
				for (uint32 j = 0; j < fbxMesh->GetPolygonSize(i); j++) {
					int32 ControlPointIdx = fbxMesh->GetPolygonVertex(i, j);

					phVertices[VertexIdxCounter].Normal.X = FbxNormal->GetDirectArray().GetAt(VertexIdxCounter).mData[0];
					phVertices[VertexIdxCounter].Normal.Y = FbxNormal->GetDirectArray().GetAt(VertexIdxCounter).mData[1];
					phVertices[VertexIdxCounter].Normal.Z = FbxNormal->GetDirectArray().GetAt(VertexIdxCounter).mData[2];
					phVertices[VertexIdxCounter].Normal.W = FbxNormal->GetDirectArray().GetAt(VertexIdxCounter).mData[3];

					VertexIdxCounter++;
				}
			}
			assert(VertexIdxCounter == NewGeometryAsset->_vertexDataNum);

			break;
		case FbxLayerElement::EReferenceMode::eIndex:
			SS_CLASS_WARNING_LOG("\"FbxLayerElement::EReferenceMode::eIndex\" is Legacy. Use \"eIndexToDirect\".");
		case FbxLayerElement::EReferenceMode::eIndexToDirect:

			for (int32 i = 0; i < fbxMesh->GetPolygonCount(); i++) {
				for (int32 j = 0; j < fbxMesh->GetPolygonSize(i); j++) {
					int32 FbxNormalIdx = FbxNormal->GetIndexArray().GetAt(VertexIdxCounter);

					phVertices[VertexIdxCounter].Normal.X = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[0];
					phVertices[VertexIdxCounter].Normal.Y = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[1];
					phVertices[VertexIdxCounter].Normal.Z = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[2];
					phVertices[VertexIdxCounter].Normal.W = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx).mData[3];

					VertexIdxCounter++;
				}
			}
			assert(VertexIdxCounter == NewGeometryAsset->_vertexDataNum);

			break;
		default:
			free(NewGeometryAsset->_indexData);
			free(NewGeometryAsset->_vertexData);
			SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
			return nullptr;
		}

		break;
	default:
		free(NewGeometryAsset->_indexData);
		free(NewGeometryAsset->_vertexData);
		SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
		return nullptr;
	}

	// 6. Load Color
	FbxGeometryElementVertexColor* FbxColor = fbxMesh->GetElementVertexColor();
	if (FbxColor != nullptr) {
		switch (FbxColor->GetMappingMode()) {

			// TODO: �÷� ĥ�ϱ�
		}
	}

	uint32 vetexDataNum = NewGeometryAsset->_vertexDataNum;
	for (uint32 i = 0; i < vetexDataNum; i++) {

		phVertices[i].Color = Vector4f(1, 1, 1, 1);
		phVertices[i].Color = Vector4f(1, 1, 1, 1);
		phVertices[i].Color = Vector4f(1, 1, 1, 1);
		phVertices[i].Color = Vector4f(1, 1, 1, 1);
	}
	
	NewGeometryAsset->_assetPath = _filePath;
	NewGeometryAsset->_assetName = _FBXImporter->GetFileName().Buffer();
	NewGeometryAsset->_assetName += fbxMesh->GetNode()->GetName();

	return NewGeometryAsset;
}

