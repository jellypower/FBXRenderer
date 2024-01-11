
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
				// Vertex를 fbx의 control point 그대로 저장하고 있음.
				// 내가 중복저장한 Vertex의 위치와 원본의 Vertex가 같음
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
				// Vertex를 중복저장해야 하고 있음. 
				// 내가 중복저장한 Vertex의 위치와 원본의 Vertex의 위치가 다름
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

			// NOTE: GetPolygonVertex함수는 실제 IndexArray에 들어가서 그리는 것을 위한 Index를 반환해 주는 것이다.
			// 그러니까 이놈으로 노말 정보를 매핑하면 원하는 정보가 나오지 않는 것은 당연하다.
			// Index버퍼 기준으로 VertexBuffer를 샘플링 하는데 순서가 바르게 나올리가 없지

			// 심지어 위에서 Vertex Index 버퍼 로딩할 때 fbxMesh->GetPolygonVertex(i, j); 로 로딩해서
			// 이미 정점 순서는 맞춰져있잖아? 그러니까 이미 맞춰준 순서인데 다시 GetPolygonVertex(i, j); 해줄 필요가 없지

			// input layout이 잘못 지정돼서 한 칸 씩 밀려 들어가는게 문제인 것으로 보인다. -> 가령 Normal의 W값이 Color의 X값으로 밀려간다거나 하는 상황.
			// 이제 해결하기 쉽겠지?

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

			// TODO: 컬러 칠하기
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

