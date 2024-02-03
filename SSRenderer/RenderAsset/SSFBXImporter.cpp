#include "SSFBXImporter.h"

#include "SSEngineDefault/SSDebugLogger.h"
#include "SSMaterialAssetManager.h"
#include "SSGeometryAssetManager.h"
#include "SSModelAssetManager.h"
#include "SSModelCombinationAssetManager.h"
#include "SSShaderAssetManager.h"
#include "AssetType/SSModelCombinationAsset.h"
#include "AssetType/SSMaterialAssetDetail/SSPbrMaterialAsset.h"

#include "capnp/message.h"
#include "capnp/compat/json.h"
#include "Serializable/SSPbrMaterialData.capnp.h"
#include "Serializable/SSTextureAssetManagingList.capnp.h"
#include "SSRenderer/SSSamplerPool.h"

bool SSFBXImporter::g_exportSSMaterial = false;

void ExtractFileNameFromFilePath(
	SS::FixedStringA<ASSET_NAME_LEN_MAX>& OutFileName,
	const char* InFilePath)
{
	const char* fileNameStart = strrchr(InFilePath, '/');
	if (fileNameStart == nullptr)
		fileNameStart = strrchr(InFilePath, '\\');

	OutFileName = fileNameStart + 1;
	uint32 cutOutLen = strrchr(OutFileName, '.') - OutFileName;
	OutFileName.CutOut(cutOutLen);
}

SSFBXImporter::SSFBXImporter()
{
	_FBXManager = ::FbxManager::Create();
	_IOSetting = FbxIOSettings::Create(_FBXManager, IOSROOT);
	_FBXImporter = ::FbxImporter::Create(_FBXManager, "");


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

	const FbxSystemUnit meterUnit(100);
	constexpr FbxSystemUnit::ConversionOptions conversionOptions
		= { true, true, true, true, true, true };
	meterUnit.ConvertScene(_currentScene, conversionOptions);

	_filePath = filePath;
	ExtractFileNameFromFilePath(_fileName, _filePath);

	return S_OK;

}

void SSFBXImporter::ClearFBXModelAsset()
{
	_currentScene->Destroy();
	_currentScene = nullptr;
	_fileName.Clear();
	_filePath.Clear();
}

void SSFBXImporter::StoreCurrentFBXModelAssetToAssetManager()
{
	ImportCurrentSceneToMaterialAsset();
	SS_LOG("========================================================\n");
	ImportCurrentSceneToModelAsset();
}


void SSFBXImporter::ImportCurrentSceneToMaterialAsset()
{

	SSShaderAsset* shaderAsset = SSShaderAssetManager::FindShaderWithName("SSDefaultPbr");
	SS::FixedStringA<ASSET_NAME_LEN_MAX> outAssetName;
	char outAssetID[10];
	const uint32 matCnt = _currentScene->GetMaterialCount();


	for (uint32 i = 0; i < matCnt; i++) {
		FbxSurfaceMaterial* material = _currentScene->GetMaterial(i);

		SS_LOG("material name: %s, shading model: %s unique id: %llu\n",
			material->GetNameOnly().Buffer(),
			material->ShadingModel.Get().Buffer(),
			material->GetUniqueID());


		_i64toa(material->GetUniqueID(), outAssetID, 10);
		outAssetName = material->GetNameOnly().Buffer();
		outAssetName += "_";
		outAssetName += outAssetID;

		SSPbrMaterialAsset* pbrMaterial = DBG_NEW SSPbrMaterialAsset(outAssetName, "SSDefaultPbr");

		FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
		if (prop.IsValid())
		{
			const int texCnt = prop.GetSrcObjectCount();
			if (texCnt != 0)
			{
				const FbxFileTexture* fbxTexture = prop.GetSrcObject<FbxFileTexture>();
				ExtractFileNameFromFilePath(outAssetName, fbxTexture->GetFileName());
				SS_LOG("\t%s name: %s\n", FbxSurfaceMaterial::sDiffuse, outAssetName.GetData());
				pbrMaterial->_textureNames[TX_BASE_COLOR_IDX] = outAssetName;
			}
		}

		prop = material->FindProperty(FbxSurfaceMaterial::sNormalMap);
		if (prop.IsValid())
		{
			const int texCnt = prop.GetSrcObjectCount();
			if (texCnt != 0)
			{
				const FbxFileTexture* fbxTexture = prop.GetSrcObject<FbxFileTexture>();
				ExtractFileNameFromFilePath(outAssetName, fbxTexture->GetFileName());
				SS_LOG("\t%s name: %s\n", FbxSurfaceMaterial::sNormalMap, outAssetName.GetData());
				pbrMaterial->_textureNames[TX_NORMAL_IDX] = outAssetName;
			}
		}

		prop = material->FindProperty(FbxSurfaceMaterial::sEmissive);
		if (prop.IsValid())
		{
			const int texCnt = prop.GetSrcPropertyCount();
			if (texCnt != 0)
			{
				const FbxFileTexture* fbxTexture = prop.GetSrcObject<FbxFileTexture>();
				ExtractFileNameFromFilePath(outAssetName, fbxTexture->GetFileName());
				SS_LOG("\t%s name: %s\n", FbxSurfaceMaterial::sEmissive, outAssetName.GetData());
				pbrMaterial->_textureNames[TX_EMISSIVE_IDX] = outAssetName;
			}
		}

		pbrMaterial->_sampleCache[0] = SSSamplerPool::Get()->GetSampler(TexFilterMode::Linear, TexAddressMode::Repeat, TexAddressMode::Repeat);

		SSMaterialAssetManager::Get()->InsertNewMaterial(pbrMaterial);
	}
}


void SSFBXImporter::ImportCurrentSceneToModelAsset()
{
	if (_currentScene == nullptr) {
		SS_CLASS_WARNING_LOG("No scene to load");
		return;
	}


	FbxNode* rootNode = _currentScene->GetRootNode();
	uint32 childCount = rootNode->GetChildCount();


	SS::FixedStringA<ASSET_NAME_LEN_MAX> assetName;
	assetName = _fileName;
	assetName += ".mdlc";
	SSModelCombinationAsset* newModelCombAsset = new SSModelCombinationAsset(assetName, (SSModelAsset*)nullptr, childCount);


	for (uint32 i = 0; i < rootNode->GetChildCount(); i++) {
		TraverseNodesRecursion(rootNode->GetChild(i), newModelCombAsset);
	}

	SSModelCombinationAssetManager::Get()->InsertNewAsset(newModelCombAsset);
}

void SSFBXImporter::TraverseNodesRecursion(::FbxNode* node, SSPlaceableAsset* parentAsset)
{
	uint32 childCount = node->GetChildCount();
	SSPlaceableAsset* thisAsset = nullptr;

	if (node->GetNodeAttribute() != nullptr) {
		switch (node->GetNodeAttribute()->GetAttributeType())
		{
		case FbxNodeAttribute::eMesh:
		{
			// geometry asset creation
			FbxMesh* fbxMesh = node->GetMesh();
			SSGeometryAsset* newGeometry = GenerateGeometryFromFbxMesh(fbxMesh);

			// model asset creation
			SS::FixedStringA<ASSET_NAME_LEN_MAX> assetName;
			assetName += _fileName;
			assetName += "_";
			assetName += fbxMesh->GetNode()->GetName();
			assetName += ".mdl";
			SSModelAsset* newModel = DBG_NEW SSModelAsset(assetName, newGeometry);


			char outAssetID[10];
			const uint32 matCnt = node->GetMaterialCount();
			for (uint32 i = 0; i < matCnt; i++) {

				FbxSurfaceMaterial* material = node->GetMaterial(i);
				_i64toa(material->GetUniqueID(), outAssetID, 10);
				assetName = material->GetNameOnly().Buffer();
				assetName += "_";
				assetName += outAssetID;


				SSMaterialAsset* modelMaterial = SSMaterialAssetManager::FindAssetWithName(assetName);
				if (modelMaterial == nullptr)
					modelMaterial = SSMaterialAssetManager::GetEmptyAsset();

				newModel->SetMaterial(modelMaterial, i);

			}

			// model combination asset creation
			assetName = fbxMesh->GetNode()->GetName();
			SSModelCombinationAsset* newModelCombAsset;
			thisAsset = newModelCombAsset = DBG_NEW SSModelCombinationAsset(assetName, newModel, childCount);

			//			newModelCombAsset->_modelAsset = newModel;

			SSGeometryAssetManager::Get()->InsertNewGeometry(newGeometry);
			SSModelAssetManager::Get()->InsertNewModel(newModel);


			// HACK:


			SS_LOG("mesh name: %s\n", node->GetName());
			SS_LOG("\tnode ID: %llu, mesh ID: %llu, uv Cnt: %d, material count: %d\n",
				fbxMesh->GetNode()->GetUniqueID(),
				fbxMesh->GetUniqueID(),
				fbxMesh->GetUVLayerCount(),
				node->GetMaterialCount()
			);

			SS_LOG("\tmateria IDs: ");
			for (uint32 i = 0; i < node->GetMaterialCount(); i++)
			{
				SS_LOG("%llu, ", node->GetMaterial(i)->GetUniqueID());
			}
			SS_LOG("\n");

			if (fbxMesh->GetElementNormal())
			{
				SS_LOG("\tnormal count: %d, ", fbxMesh->GetElementNormal()->GetDirectArray().GetCount());
			}
			if (fbxMesh->GetElementUV())
			{
				SS_LOG("uv count: %d, ", fbxMesh->GetElementUV()->GetDirectArray().GetCount());
			}
			SS_LOG("ctrl count: %d, ", fbxMesh->GetControlPointsCount());
			SS_LOG("\n");

			switch (fbxMesh->GetElementMaterial()->GetMappingMode())
			{
			case FbxLayerElement::eByPolygon:


				FbxLayerElementArrayTemplate<int>* materialIndices;
				fbxMesh->GetMaterialIndices(&materialIndices);

				SS_LOG("\t(By Polygon) material indice count: %d, polygon count: %d\n", materialIndices->GetCount(), fbxMesh->GetPolygonCount());
				break;
			case FbxLayerElement::eAllSame:
				SS_LOG("\t(All Same)\n");
				break;
			default:
				assert(false);
				break;
			}


			SS_LOG("\n\n");
			// HACKEND:
		}
		break;
		default:
		{
			thisAsset = DBG_NEW SSPlaceableAsset(AssetType::Blank, childCount);
		}
		break;
		}


	}

	if (thisAsset == nullptr)
		thisAsset = DBG_NEW SSPlaceableAsset(AssetType::Blank, childCount);


	FbxAMatrix fbxMat;
	fbxMat.SetIdentity();
	fbxMat.SetT(node->GetGeometricTranslation(FbxNode::eSourcePivot));
	fbxMat.SetR(node->GetGeometricRotation(FbxNode::eSourcePivot));
	fbxMat.SetS(node->GetGeometricScaling(FbxNode::eSourcePivot));

	fbxMat = node->EvaluateLocalTransform() * fbxMat;


	assert(thisAsset != nullptr);
	const FbxDouble3 fbxTranslate = fbxMat.GetT();
	thisAsset->_transform.Position.X = -fbxTranslate.mData[0];
	thisAsset->_transform.Position.Y = fbxTranslate.mData[1];
	thisAsset->_transform.Position.Z = fbxTranslate.mData[2];
	thisAsset->_transform.Position.W = 0;

	const FbxDouble3 fbxScale = fbxMat.GetS();
	thisAsset->_transform.Scale.X = fbxScale.mData[0];
	thisAsset->_transform.Scale.Y = fbxScale.mData[1];
	thisAsset->_transform.Scale.Z = fbxScale.mData[2];
	thisAsset->_transform.Scale.W = 0;

	const FbxDouble3 fbxRotation = fbxMat.GetR(); // pitch roll yaw

	Quaternion curRotation;
	curRotation = Quaternion::RotateAxisAngle(curRotation, Vector4f::Right, SSStaticMath::DegToRadians(fbxRotation.mData[0]));
	curRotation = Quaternion::RotateAxisAngle(curRotation, Vector4f::Up, -SSStaticMath::DegToRadians(fbxRotation.mData[1]));
	curRotation = Quaternion::RotateAxisAngle(curRotation, Vector4f::Forward, -SSStaticMath::DegToRadians(fbxRotation.mData[2]));

	thisAsset->_transform.Rotation = curRotation;

	thisAsset->_parent = parentAsset;
	parentAsset->_childs.PushBack(thisAsset);
	for (uint32 i = 0; i < childCount; i++) {
		TraverseNodesRecursion(node->GetChild(i), thisAsset);
	}
}



SSGeometryAsset* SSFBXImporter::GenerateGeometryFromFbxMesh(::FbxMesh* fbxMesh)
{
	assert(fbxMesh != nullptr);
	SSGeometryAsset* NewGeometryAsset = DBG_NEW SSGeometryAsset();

	// 1. Load num
	const uint32 layerNum = fbxMesh->GetLayerCount();
	const uint32 ControlPointNum = fbxMesh->GetControlPointsCount();
	const uint32 PolygonCount = fbxMesh->GetPolygonCount();
	const uint32 PolygonVertexNum = fbxMesh->GetPolygonVertexCount(); // sum of vertex in each polygon
	const FbxGeometryElementNormal* const FbxNormal = fbxMesh->GetElementNormal();
	SS_ASSERT(FbxNormal != nullptr, "normal must be exists.");

	uint32 uvChannelCnt = fbxMesh->GetUVLayerCount();
	FbxGeometryElementUV* fbxUV[2];
	if (uvChannelCnt > 2)
	{
		SS_CLASS_WARNING_LOG("Too many uv channel");
		uvChannelCnt = 2;
	}
	for (uint32 i = 0; i < uvChannelCnt; i++)
	{
		fbxUV[i] = fbxMesh->GetElementUV(i);
		SS_ASSERT(fbxUV[i] != nullptr, "uv must be exists of idx %d", i);
	}


	// *calculate vertex unit
	NewGeometryAsset->_vertexUnit = EVertexUnit::VertexPerPoint;
	if (FbxNormal->GetMappingMode() == FbxLayerElement::eByPolygonVertex)
		NewGeometryAsset->_vertexUnit = EVertexUnit::VertexPerPolygon;
	if (uvChannelCnt > 0 && fbxUV[0]->GetMappingMode() == FbxLayerElement::eByPolygonVertex)
		NewGeometryAsset->_vertexUnit = EVertexUnit::VertexPerPolygon;
	if (uvChannelCnt > 1 && fbxUV[1]->GetMappingMode() == FbxLayerElement::eByPolygonVertex)
		NewGeometryAsset->_vertexUnit = EVertexUnit::VertexPerPolygon;

	NewGeometryAsset->_drawTopologyType = EGeometryDrawTopology::TriangleList;

	// 2. alloc vertex memory
	switch (NewGeometryAsset->_vertexUnit) {

	case EVertexUnit::VertexPerPoint:
		NewGeometryAsset->_vertexDataNum = ControlPointNum; break;
	case EVertexUnit::VertexPerPolygon:
		NewGeometryAsset->_vertexDataNum = PolygonVertexNum; break;

	}


	NewGeometryAsset->_eachVertexDataSize = sizeof(SSDefaultVertex);
	NewGeometryAsset->_vertexData = (SSDefaultVertex*)DBG_MALLOC(NewGeometryAsset->_eachVertexDataSize * NewGeometryAsset->_vertexDataNum);
	SSDefaultVertex* phVertices = NewGeometryAsset->_vertexData;

	// 3. alloc index memory
	NewGeometryAsset->_subGeometryNum = fbxMesh->GetNode()->GetMaterialCount();
	assert(NewGeometryAsset->_subGeometryNum < SUBGEOM_COUNT_MAX);
	FbxLayerElementArrayTemplate<int>* materialIndices = nullptr;

	switch (fbxMesh->GetElementMaterial()->GetMappingMode())
	{
	case FbxLayerElement::eAllSame:
	{
		for (uint32 i = 0; i < PolygonCount; i++)
		{
			NewGeometryAsset->_indexDataNum[0] += (fbxMesh->GetPolygonSize(i) - 2);
		}
		NewGeometryAsset->_indexDataNum[0] *= 3;
	}
	break;

	case FbxLayerElement::eByPolygon:
	{
		fbxMesh->GetMaterialIndices(&materialIndices);

		for (uint32 i = 0; i < PolygonCount; i++)
		{
			uint8 matIdx = materialIndices->GetAt(i);
			NewGeometryAsset->_indexDataNum[matIdx] += ((fbxMesh->GetPolygonSize(i) - 2) * 3);
		}

	}
	break;
	default:
		assert(false);
	}

	uint32 idxAcc = 0;
	for (uint32 i = 0; i < NewGeometryAsset->_subGeometryNum; i++)
	{
		NewGeometryAsset->_indexDataStartIndex[i] = idxAcc;
		idxAcc += NewGeometryAsset->_indexDataNum[i];
	}
	NewGeometryAsset->_wholeIndexDataNum = idxAcc;
	NewGeometryAsset->_indexData = (uint32*)DBG_MALLOC(sizeof(uint32) * NewGeometryAsset->_wholeIndexDataNum);




	// 4. Load vertex/Index data
	FbxVector4* ControlPoints = fbxMesh->GetControlPoints();
	uint32 CurIndexDataIdx[SUBGEOM_COUNT_MAX] = { 0, };

	switch (NewGeometryAsset->_vertexUnit) {

	case EVertexUnit::VertexPerPoint:
	{

		for (int32 i = 0; i < fbxMesh->GetControlPointsCount(); i++) {
			phVertices[i].Pos.X = -ControlPoints[i].mData[0];
			phVertices[i].Pos.Y = ControlPoints[i].mData[1];
			phVertices[i].Pos.Z = ControlPoints[i].mData[2];
			phVertices[i].Pos.W = ControlPoints[i].mData[3];
		}

		switch (fbxMesh->GetElementMaterial()->GetMappingMode())
		{

		case FbxLayerElement::eAllSame:
		{
			for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++)
			{
				uint32 thisPolygonSize = fbxMesh->GetPolygonSize(i);
				for (uint32 j = 1; j < thisPolygonSize - 1; j++)
				{
					NewGeometryAsset->_indexData[CurIndexDataIdx[0]++] = fbxMesh->GetPolygonVertex(i, j + 1);
					NewGeometryAsset->_indexData[CurIndexDataIdx[0]++] = fbxMesh->GetPolygonVertex(i, j);
					NewGeometryAsset->_indexData[CurIndexDataIdx[0]++] = fbxMesh->GetPolygonVertex(i, 0);
				}
			}
		}
		break;
		case FbxLayerElement::eByPolygon:
		{
			for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++)
			{
				uint32 thisPolygonSize = fbxMesh->GetPolygonSize(i);
				uint32 matIdx = materialIndices->GetAt(i);
				uint32 idxDataStart = NewGeometryAsset->_indexDataStartIndex[matIdx];
				for (uint32 j = 1; j < thisPolygonSize - 1; j++)
				{
					NewGeometryAsset->_indexData[idxDataStart + CurIndexDataIdx[matIdx]++] = fbxMesh->GetPolygonVertex(i, j + 1);
					NewGeometryAsset->_indexData[idxDataStart + CurIndexDataIdx[matIdx]++] = fbxMesh->GetPolygonVertex(i, j);
					NewGeometryAsset->_indexData[idxDataStart + CurIndexDataIdx[matIdx]++] = fbxMesh->GetPolygonVertex(i, 0);
				}
			}
		}
		break;
		}
		for (uint32 i = 0; i < NewGeometryAsset->_subGeometryNum; i++)
			assert(NewGeometryAsset->_indexDataNum[i] == CurIndexDataIdx[i]);
	}
	break;
	case EVertexUnit::VertexPerPolygon:
	{
		int32 VertexIdxCounter = 0;
		uint32 fbxMeshPolygonCount = fbxMesh->GetPolygonCount();

		for (int32 i = 0; i < fbxMeshPolygonCount; i++) {
			uint32 thisPolygonVerticesCount = fbxMesh->GetPolygonSize(i);
			for (int32 j = 0; j < thisPolygonVerticesCount; j++) {
				int32 ControlPointIdx = fbxMesh->GetPolygonVertex(i, j);
				FbxVector4 vertexPos = fbxMesh->GetControlPointAt(ControlPointIdx);
				phVertices[VertexIdxCounter].Pos.X = -vertexPos.mData[0];
				phVertices[VertexIdxCounter].Pos.Y = vertexPos.mData[1];
				phVertices[VertexIdxCounter].Pos.Z = vertexPos.mData[2];
				phVertices[VertexIdxCounter].Pos.W = vertexPos.mData[3];

				VertexIdxCounter++;
			}
		}
		assert(VertexIdxCounter == PolygonVertexNum);

		switch (fbxMesh->GetElementMaterial()->GetMappingMode())
		{
		case FbxLayerElement::eAllSame:
		{
			for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++)
			{
				uint32 thisPolygonSize = fbxMesh->GetPolygonSize(i);
				for (uint32 j = 1; j < thisPolygonSize - 1; j++)
				{
					NewGeometryAsset->_indexData[CurIndexDataIdx[0]++] = fbxMesh->GetPolygonVertexIndex(i) + j + 1;
					NewGeometryAsset->_indexData[CurIndexDataIdx[0]++] = fbxMesh->GetPolygonVertexIndex(i) + j;
					NewGeometryAsset->_indexData[CurIndexDataIdx[0]++] = fbxMesh->GetPolygonVertexIndex(i);
				}
			}
		}
		break;
		case FbxLayerElement::eByPolygon:
		{
			for (uint32 i = 0; i < PolygonCount; i++)
			{
				uint32 thisPolygonSize = fbxMesh->GetPolygonSize(i);
				uint32 matIdx = materialIndices->GetAt(i);
				uint32 idxDataStart = NewGeometryAsset->_indexDataStartIndex[matIdx];
				for (uint32 j = 1; j < thisPolygonSize - 1; j++)
				{
					NewGeometryAsset->_indexData[idxDataStart + CurIndexDataIdx[matIdx]++] = fbxMesh->GetPolygonVertexIndex(i) + j + 1;
					NewGeometryAsset->_indexData[idxDataStart + CurIndexDataIdx[matIdx]++] = fbxMesh->GetPolygonVertexIndex(i) + j;
					NewGeometryAsset->_indexData[idxDataStart + CurIndexDataIdx[matIdx]++] = fbxMesh->GetPolygonVertexIndex(i);
				}
			}
		}
		break;
		}
		for (uint32 i = 0; i < NewGeometryAsset->_subGeometryNum; i++)
			assert(NewGeometryAsset->_indexDataNum[i] == CurIndexDataIdx[i]);
	}
	break;
	default:
		free(NewGeometryAsset->_indexData);
		free(NewGeometryAsset->_vertexData);
		SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
		return nullptr;
	}


	// 5. Load normal data
	switch (NewGeometryAsset->_vertexUnit) {
	case EVertexUnit::VertexPerPoint:

		switch (FbxNormal->GetReferenceMode()) {
		case FbxLayerElement::eDirect:

			for (int32 i = 0; i < fbxMesh->GetControlPointsCount(); i++) {

				FbxVector4 normalVector = FbxNormal->GetDirectArray().GetAt(i);
				normalVector.Normalize();

				phVertices[i].Normal.X = -normalVector.mData[0];
				phVertices[i].Normal.Y = normalVector.mData[1];
				phVertices[i].Normal.Z = normalVector.mData[2];
				phVertices[i].Normal.W = normalVector.mData[3];
				phVertices[i].Normal = phVertices[i].Normal.GetNormalized();
			}

			break;
		case FbxLayerElement::eIndex:
			SS_CLASS_WARNING_LOG("\"FbxLayerElement::EReferenceMode::eIndex\" is Legacy. Use \"eIndexToDirect\".");
		case FbxLayerElement::eIndexToDirect:

			for (int32 i = 0; i < fbxMesh->GetControlPointsCount(); i++) {
				int32 FbxNormalIdx = FbxNormal->GetIndexArray().GetAt(i);
				FbxVector4 normalVector = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx);

				phVertices[i].Normal.X = -normalVector.mData[0];
				phVertices[i].Normal.Y = normalVector.mData[1];
				phVertices[i].Normal.Z = normalVector.mData[2];
				phVertices[i].Normal.W = normalVector.mData[3];
				phVertices[i].Normal = phVertices[i].Normal.GetNormalized();
			}

			break;
		default:
			free(NewGeometryAsset->_indexData);
			free(NewGeometryAsset->_vertexData);
			SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
			return nullptr;
		}

		break;
	case EVertexUnit::VertexPerPolygon:
	{
		int32 VertexIdxCounter = 0;

		switch (FbxNormal->GetReferenceMode()) {
		case FbxLayerElement::eDirect:

			for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++) {
				for (uint32 j = 0; j < fbxMesh->GetPolygonSize(i); j++) {

					int32 ControlPointIdx;
					if (FbxNormal->GetMappingMode() == FbxLayerElement::eByPolygonVertex) ControlPointIdx = fbxMesh->GetPolygonVertexIndex(i) + j;
					else ControlPointIdx = fbxMesh->GetPolygonVertex(i, j);

					FbxVector4 normalVector = FbxNormal->GetDirectArray().GetAt(ControlPointIdx);
					normalVector.Normalize();
					phVertices[VertexIdxCounter].Normal.X = -normalVector.mData[0];
					phVertices[VertexIdxCounter].Normal.Y = normalVector.mData[1];
					phVertices[VertexIdxCounter].Normal.Z = normalVector.mData[2];
					phVertices[VertexIdxCounter].Normal.W = normalVector.mData[3];
					phVertices[VertexIdxCounter].Normal = phVertices[VertexIdxCounter].Normal.GetNormalized();

					VertexIdxCounter++;
				}
			}
			assert(VertexIdxCounter == NewGeometryAsset->_vertexDataNum);

			break;
		case FbxLayerElement::eIndex:
			SS_CLASS_WARNING_LOG("\"FbxLayerElement::EReferenceMode::eIndex\" is Legacy. Use \"eIndexToDirect\".");
		case FbxLayerElement::eIndexToDirect:

			for (int32 i = 0; i < fbxMesh->GetPolygonCount(); i++) {
				for (int32 j = 0; j < fbxMesh->GetPolygonSize(i); j++) {

					int32 ControlPointIdx;
					if (FbxNormal->GetMappingMode() == FbxLayerElement::eByPolygonVertex) ControlPointIdx = fbxMesh->GetPolygonVertexIndex(i) + j;
					else ControlPointIdx = fbxMesh->GetPolygonVertex(i, j);


					uint32 normalIdx = FbxNormal->GetIndexArray().GetAt(ControlPointIdx);
					FbxVector4 normalVector = FbxNormal->GetDirectArray().GetAt(normalIdx);
					normalVector.Normalize();

					phVertices[VertexIdxCounter].Normal.X = -normalVector.mData[0];
					phVertices[VertexIdxCounter].Normal.Y = normalVector.mData[1];
					phVertices[VertexIdxCounter].Normal.Z = normalVector.mData[2];
					phVertices[VertexIdxCounter].Normal.W = normalVector.mData[3];
					phVertices[VertexIdxCounter].Normal = phVertices[VertexIdxCounter].Normal.GetNormalized();

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
	}
	break;
	default:
		free(NewGeometryAsset->_indexData);
		free(NewGeometryAsset->_vertexData);
		SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
		return nullptr;
	}

	// 6. Load UV
	for (uint32 uvIdx = 0; uvIdx < uvChannelCnt; uvIdx++)
	{
		int32 VertexIdxCounter = 0;
		FbxGeometryElementUV* fbxUVItem = fbxMesh->GetElementUV(uvIdx);
		assert(fbxUVItem != nullptr);

		switch (NewGeometryAsset->_vertexUnit)
		{
		case EVertexUnit::VertexPerPoint:
			switch (fbxUVItem->GetReferenceMode())
			{
			case FbxLayerElement::eDirect:

				for (int32 i = 0; i < fbxMesh->GetControlPointsCount(); i++) {
					FbxVector2 uvVector = fbxUVItem->GetDirectArray().GetAt(i);

					phVertices[i].Uv[uvIdx].X = uvVector.mData[0];
					phVertices[i].Uv[uvIdx].Y = 1 - uvVector.mData[1];
				}

				break;
			case FbxLayerElement::eIndex:
				SS_CLASS_WARNING_LOG("\"FbxLayerElement::EReferenceMode::eIndex\" is Legacy. Use \"eIndexToDirect\".");
			case FbxLayerElement::eIndexToDirect:

				for (int32 i = 0; i < fbxMesh->GetControlPointsCount(); i++) {
					int32 FbxUvIdx = fbxUVItem->GetIndexArray().GetAt(i);
					FbxVector2 uvVector = fbxUVItem->GetDirectArray().GetAt(FbxUvIdx);

					phVertices[i].Uv[uvIdx].X = uvVector.mData[0];
					phVertices[i].Uv[uvIdx].Y = 1 - uvVector.mData[1];
				}

				break;
			default:
				free(NewGeometryAsset->_indexData);
				free(NewGeometryAsset->_vertexData);
				SS_CLASS_ERR_LOG("Invalid Fbxformat. function just return");
				return nullptr;
			}
			break;
			//=========================================================================================================
		case EVertexUnit::VertexPerPolygon:

			switch (fbxUVItem->GetReferenceMode())
			{
			case FbxLayerElement::eDirect:

				for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++) {
					uint32 thisPolygonSize = fbxMesh->GetPolygonSize(i);
					for (uint32 j = 0; j < thisPolygonSize; j++) {

						uint32 uvIdx = fbxMesh->GetTextureUVIndex(i, j);
						FbxVector2 uvVector = fbxUVItem->GetDirectArray().GetAt(uvIdx);

						phVertices[VertexIdxCounter].Uv[uvIdx].X = uvVector.mData[0];
						phVertices[VertexIdxCounter].Uv[uvIdx].Y = 1 - uvVector.mData[1];

						VertexIdxCounter++;
					}
				}
				assert(VertexIdxCounter == NewGeometryAsset->_vertexDataNum);

				break;
			case FbxLayerElement::eIndex:
				SS_CLASS_WARNING_LOG("\"FbxLayerElement::EReferenceMode::eIndex\" is Legacy. Use \"eIndexToDirect\".");
			case FbxLayerElement::eIndexToDirect:

				for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++) {
					uint32 thisPolygonSize = fbxMesh->GetPolygonSize(i);
					for (uint32 j = 0; j < thisPolygonSize; j++) {
						fbxMesh->GetPolygonVertex(i, j);
						uint32 polygonVertexIdx = fbxMesh->GetTextureUVIndex(i, j);
						FbxVector2 uvVector = fbxUVItem->GetDirectArray().GetAt(polygonVertexIdx);

						phVertices[VertexIdxCounter].Uv[uvIdx].X = uvVector.mData[0];
						phVertices[VertexIdxCounter].Uv[uvIdx].Y = 1 - uvVector.mData[1];

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
	}


	FbxGeometryElementTangent* fbxTangent = fbxMesh->GetElementTangent();
	uint32 cnt = fbxMesh->GetElementTangentCount();
	cnt = fbxMesh->GetElementBinormalCount();
	// load tangent
	if (fbxTangent != nullptr) {
		uint32 VertexIdxCounter = 0;

		switch (NewGeometryAsset->_vertexUnit)
		{
		case EVertexUnit::VertexPerPoint:
			switch (fbxTangent->GetReferenceMode())
			{
			case FbxLayerElement::eDirect:
				for (uint32 i = 0; i < fbxMesh->GetControlPointsCount(); i++)
				{
					FbxVector4 tanVector = fbxTangent->GetDirectArray().GetAt(i);

					phVertices[i].Tangent.X = tanVector.mData[0];
					phVertices[i].Tangent.Y = tanVector.mData[1];
					phVertices[i].Tangent.Z = tanVector.mData[2];
					phVertices[i].Tangent.W = tanVector.mData[3];
				}
				break;
			case FbxLayerElement::eIndex:
				SS_CLASS_WARNING_LOG("\"FbxLayerElement::EReferenceMode::eIndex\" is Legacy. Use \"eIndexToDirect\".");
			case FbxLayerElement::eIndexToDirect:
				for (int32 i = 0; i < fbxMesh->GetControlPointsCount(); i++) {
					int32 tangentIdx = fbxTangent->GetIndexArray().GetAt(i);
					FbxVector4 tanVector = fbxTangent->GetDirectArray().GetAt(tangentIdx);

					phVertices[i].Tangent.X = -tanVector.mData[0];
					phVertices[i].Tangent.Y = tanVector.mData[1];
					phVertices[i].Tangent.Z = tanVector.mData[2];
					phVertices[i].Tangent.W = tanVector.mData[3];
				}
				break;
			}
			break;

		case EVertexUnit::VertexPerPolygon:
			VertexIdxCounter = 0;

			switch (fbxTangent->GetReferenceMode())
			{
			case FbxLayerElement::eDirect:

				for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++) {
					for (uint32 j = 0; j < fbxMesh->GetPolygonSize(i); j++) {
						int32 ControlPointIdx;
						if (fbxTangent->GetMappingMode() == FbxLayerElement::eByPolygonVertex) ControlPointIdx = fbxMesh->GetPolygonVertexIndex(i) + j;
						else ControlPointIdx = fbxMesh->GetPolygonVertex(i, j);

						FbxVector4 tanVector = fbxTangent->GetDirectArray().GetAt(ControlPointIdx);
						tanVector.Normalize();
						phVertices[VertexIdxCounter].Tangent.X = -tanVector.mData[0];
						phVertices[VertexIdxCounter].Tangent.Y = tanVector.mData[1];
						phVertices[VertexIdxCounter].Tangent.Z = tanVector.mData[2];
						phVertices[VertexIdxCounter].Tangent.W = tanVector.mData[3];

						VertexIdxCounter++;
					}
				}
				assert(VertexIdxCounter == NewGeometryAsset->_vertexDataNum);
				break;

			case FbxLayerElement::eIndex:
				SS_CLASS_WARNING_LOG("\"FbxLayerElement::EReferenceMode::eIndex\" is Legacy. Use \"eIndexToDirect\".");
			case FbxLayerElement::eIndexToDirect:
				for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++) {
					for (uint32 j = 0; j < fbxMesh->GetPolygonSize(i); j++) {
						uint32 ControlPointIdx;
						if (fbxTangent->GetMappingMode() == FbxLayerElement::eByPolygonVertex) ControlPointIdx = fbxMesh->GetPolygonVertexIndex(i) + j;
						else ControlPointIdx = fbxMesh->GetPolygonVertex(i, j);

						uint32 tangentIdx = fbxTangent->GetIndexArray().GetAt(ControlPointIdx);
						FbxVector4 tanVector = fbxTangent->GetDirectArray().GetAt(tangentIdx);
						tanVector.Normalize();
						phVertices[VertexIdxCounter].Tangent.X = -tanVector.mData[0];
						phVertices[VertexIdxCounter].Tangent.Y = tanVector.mData[1];
						phVertices[VertexIdxCounter].Tangent.Z = tanVector.mData[2];
						phVertices[VertexIdxCounter].Tangent.W = tanVector.mData[3];

						VertexIdxCounter++;
					}
				}
				assert(VertexIdxCounter == NewGeometryAsset->_vertexDataNum);
				break;
			}
			break;
		}
	}
	else
	{
		
		for (uint32 subGeomIdx = 0; subGeomIdx < NewGeometryAsset->_subGeometryNum; subGeomIdx++)
		{
			uint32* thisIdxData = NewGeometryAsset->_indexData + NewGeometryAsset->_indexDataStartIndex[subGeomIdx];
			uint32 thisIdxDataNum = NewGeometryAsset->_indexDataNum[subGeomIdx];
			assert(thisIdxDataNum % 3 == 0);

			for (uint32 i = 0; i < thisIdxDataNum; i += 3)
			{
				SSDefaultVertex& v0 = phVertices[thisIdxData[i]];
				SSDefaultVertex& v1 = phVertices[thisIdxData[i + 1]];
				SSDefaultVertex& v2 = phVertices[thisIdxData[i + 2]];

				Vector4f dv1 = v1.Pos - v0.Pos;
				Vector4f dv2 = v2.Pos - v0.Pos;

				Vector2f duv1 = v1.Uv[0] - v0.Uv[0];
				Vector2f duv2 = v2.Uv[0] - v0.Uv[0];

				float detInverse = 1.0f / (duv1.X * duv2.Y - duv1.Y * duv2.X);


				Vector4f tangent = (dv1 * duv2.Y - dv2 * duv1.Y);
				tangent = tangent.GetNormalized();

				v2.Tangent = v1.Tangent = v0.Tangent = tangent;
			}
		}
		
	}

	NewGeometryAsset->_assetPath = _FBXImporter->GetFileName().Buffer();
	NewGeometryAsset->_assetName = _fileName.GetData();
	NewGeometryAsset->_assetName += "_";
	NewGeometryAsset->_assetName += fbxMesh->GetNode()->GetName();
	NewGeometryAsset->_assetName += ".geom";

	return NewGeometryAsset;
}