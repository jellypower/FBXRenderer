#include "SSFBXImporter.h"

#include "SSEngineDefault/SSDebugLogger.h"
#include "SSMaterialAssetManager.h"
#include "SSGeometryAssetManager.h"
#include "SSModelAssetManager.h"
#include "SSModelCombinationAssetManager.h"
#include "SSShaderAssetManager.h"
#include "SSSkeletonAssetManager.h"
#include "SSSkeletonAnimAssetManager.h"
#include "SSTextureAssetManager.h"
#include "AssetType/SSModelCombinationAsset.h"
#include "AssetType/SSSkeletonAsset.h"
#include "AssetType/SSMaterialAssetDetail/SSPbrMaterialAsset.h"


#include "Serializable/SSPbrMaterialData.capnp.h"
#include "Serializable/SSTextureAssetManagingList.capnp.h"
#include "SSRenderer/SSSamplerPool.h"

bool SSFBXImporter::g_exportSSMaterial = false;

namespace SSFbxName
{
	constexpr char BumpMap[] = "bump_map";
	constexpr char MetalnessMap[] = "metalness_map";
	constexpr char Metalness[] = "metalness";
	constexpr char EmissiveColor[] = "EmissiveColor";
}


void ExtractFileNameFromFilePath(SS::FixedStringA<128>& OutFileName, const char* InFilePath)
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
	_FBXImporter = ::FbxImporter::Create(_FBXManager, "");

}

SSFBXImporter::~SSFBXImporter()
{
	_FBXImporter->Destroy();
	_FBXManager->Destroy();
}

HRESULT SSFBXImporter::LoadModelAssetFromFBXFile(const utf8* filePath)
{
	if (!_FBXImporter->Initialize(filePath)) {
		SS_CLASS_WARNING_LOG("%s", _FBXImporter->GetStatus().GetErrorString());
		return E_FAIL;
	}

	_currentScene = FbxScene::Create(_FBXManager, filePath);
	_FBXImporter->Import(_currentScene);

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
	ImportCurrentSceneToSkeletonAsset();
	SS_LOG("========================================================\n");
	ImportCurrentSceneToSkeletonAnimAsset();
	SS_LOG("========================================================\n");
	ImportCurrentSceneToModelAsset();
}


void SSFBXImporter::ImportCurrentSceneToMaterialAsset()
{
	SS::FixedStringA<ASSET_NAME_LEN_MAX> outAssetName;
	char outAssetID[10];
	const uint32 matCnt = _currentScene->GetMaterialCount();


	for (uint32 i = 0; i < matCnt; i++) {
		FbxSurfaceMaterial* material = _currentScene->GetMaterial(i);

		SS_LOG("material name: %s, shading model: %s, unique id: %llu\n",
			material->GetNameOnly().Buffer(),
			material->ShadingModel.Get().Buffer(),
			material->GetUniqueID());


		_i64toa(material->GetUniqueID(), outAssetID, 10);
		outAssetName = material->GetNameOnly().Buffer();
		outAssetName += "_";
		outAssetName += outAssetID;

		SSPbrMaterialAsset* pbrMaterial = DBG_NEW SSPbrMaterialAsset(outAssetName);

		FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
		if (prop.IsValid())
		{
			const int texCnt = prop.GetSrcObjectCount();
			if (texCnt != 0)
			{
				const FbxFileTexture* fbxTexture = prop.GetSrcObject<FbxFileTexture>();
				ExtractFileNameFromFilePath(outAssetName, fbxTexture->GetFileName());
				SS_LOG("\t%s name: %s\n", prop.GetNameAsCStr(), outAssetName.C_Str());
				pbrMaterial->_textureNames[SS_PBR_TX_BASE_COLOR_IDX] = outAssetName;
			}
		}

		prop = material->FindProperty(SSFbxName::BumpMap, false);
		if (prop.IsValid() == false)
			prop = material->FindProperty(FbxSurfaceMaterial::sNormalMap);
		if (prop.IsValid())
		{
			const int texCnt = prop.GetSrcObjectCount();
			if (texCnt != 0)
			{
				const FbxFileTexture* fbxTexture = prop.GetSrcObject<FbxFileTexture>();
				ExtractFileNameFromFilePath(outAssetName, fbxTexture->GetFileName());
				SS_LOG("\t%s name: %s\n", prop.GetNameAsCStr(), outAssetName.C_Str());
				pbrMaterial->_textureNames[SS_PBR_TX_NORMAL_IDX] = outAssetName;
			}
		}



		prop = material->FindProperty(FbxSurfaceMaterial::sEmissive);
		if (prop.IsValid())
		{
			const int texCnt = prop.GetSrcObjectCount();
			if (texCnt != 0)
			{
				const FbxFileTexture* fbxTexture = prop.GetSrcObject<FbxFileTexture>();
				ExtractFileNameFromFilePath(outAssetName, fbxTexture->GetFileName());
				SS_LOG("\t%s name: %s\n", prop.GetNameAsCStr(), outAssetName.C_Str());
				pbrMaterial->_textureNames[SS_PBR_TX_EMISSIVE_IDX] = outAssetName;
			}
		}


		prop = material->FindProperty(FbxSurfaceMaterial::sDiffuseFactor);
		if (prop.IsValid())
		{
			FbxDouble3 diffuseFactor = prop.Get<FbxDouble3>();
			pbrMaterial->SetBaseColorFactor(Vector4f(diffuseFactor[0], diffuseFactor[1], diffuseFactor[2], 1));
		}


		prop = material->FindProperty(FbxSurfaceMaterial::sEmissiveFactor);
		if (prop.IsValid())
		{
			uint32 type = prop.GetPropertyDataType().GetType();
			FbxDouble emissive = prop.Get<FbxDouble>();
			if (emissive != 0.0f)
				pbrMaterial->SetEmissiveFactor(Vector4f(emissive, emissive, emissive, 1));
		}

		for (prop = material->GetFirstProperty(); prop.IsValid(); prop = material->GetNextProperty(prop))
		{
			//			SS_LOG("\t\t%s: %s\n", prop.GetNameAsCStr(), prop.GetPropertyDataType().GetName());

			if (strcmp(prop.GetNameAsCStr(), SSFbxName::BumpMap) == 0)
			{
				const FbxFileTexture* fbxTexture = prop.GetSrcObject<FbxFileTexture>();
				ExtractFileNameFromFilePath(outAssetName, fbxTexture->GetFileName());
				SS_LOG("\t%s name: %s\n", SSFbxName::BumpMap, outAssetName.C_Str());
				pbrMaterial->_textureNames[SS_PBR_TX_NORMAL_IDX] = outAssetName;
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

FbxNode* FindSkeletonRootRecursion(FbxNode* searchNode)
{
	if (searchNode->GetNodeAttribute() != nullptr &&
		searchNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton) return searchNode;

	for (uint32 i = 0; i < searchNode->GetChildCount(); i++)
	{
		FbxNode* childItem = FindSkeletonRootRecursion(searchNode->GetChild(i));
		if (childItem != nullptr &&
			childItem->GetNodeAttribute() != nullptr &&
			childItem->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			return childItem;
	}
	return nullptr;
}


Transform ExtractTransformFromNode(FbxNode* node, FbxTime fbxTime = FBXSDK_TIME_INFINITE)
{
	FbxAMatrix fbxMat;
	fbxMat.SetIdentity();
	fbxMat.SetT(node->GetGeometricTranslation(FbxNode::eSourcePivot));
	fbxMat.SetR(node->GetGeometricRotation(FbxNode::eSourcePivot));
	fbxMat.SetS(node->GetGeometricScaling(FbxNode::eSourcePivot));
	

	fbxMat = node->EvaluateLocalTransform(fbxTime) * fbxMat;

	Transform transform;

	const FbxDouble3 fbxTranslate = fbxMat.GetT();
	transform.Position.X = -fbxTranslate.mData[0] * 0.01;
	transform.Position.Y = fbxTranslate.mData[1] * 0.01;
	transform.Position.Z = fbxTranslate.mData[2] * 0.01;
	transform.Position.W = 0;

	const FbxDouble3 fbxScale = fbxMat.GetS();
	transform.Scale.X = fbxScale.mData[0];
	transform.Scale.Y = fbxScale.mData[1];
	transform.Scale.Z = fbxScale.mData[2];
	transform.Scale.W = 0;

	const FbxQuaternion fbxRotation = fbxMat.GetQ(); // pitch yaw roll ���� �ٲ���� ��
	transform.Rotation.X = fbxRotation.mData[0];
	transform.Rotation.Y = -fbxRotation.mData[1];
	transform.Rotation.Z = -fbxRotation.mData[2];
	transform.Rotation.W = fbxRotation.mData[3];


//	transform.Rotation = Quaternion::RotateAxisAngle(transform.Rotation, Vector4f::Right, SS::DegToRadians(fbxRotation.mData[1]));
//	transform.Rotation = Quaternion::RotateAxisAngle(transform.Rotation, Vector4f::Forward, -SS::DegToRadians(fbxRotation.mData[2]));
//	transform.Rotation = Quaternion::RotateAxisAngle(transform.Rotation, Vector4f::Up, SS::DegToRadians(fbxRotation.mData[0]));
	

	return transform;
}

uint32 FindFbxNodeWholeChildCnt(FbxNode* searchNode)
{
	uint32 sum = 1;
	for (uint32 i = 0; i < searchNode->GetChildCount(); i++)
	{
		sum += FindFbxNodeWholeChildCnt(searchNode->GetChild(i));
	}

	return sum;
}

void ImportBoneToSkeletonRecursion(SS::PooledList<BoneNode>& boneList, FbxNode* node, uint16 parentBoneIdx)
{
	uint32 childCnt = node->GetChildCount();

	boneList.PushBack(BoneNode(node->GetName(), parentBoneIdx, childCnt));
	uint16 boneListLastIdx = boneList.GetSize() - 1;


	BoneNode& boneNode = boneList[boneListLastIdx];
	if (parentBoneIdx != 65535)
		boneList[parentBoneIdx]._childs.PushBack(boneListLastIdx);

	boneNode._length = node->GetSkeleton()->LimbLength.Get();
	boneNode._transform = ExtractTransformFromNode(node);



	for (uint32 i = 0; i < childCnt; i++)
	{
		ImportBoneToSkeletonRecursion(boneList, node->GetChild(i), boneListLastIdx);
	}
}

void SSFBXImporter::ImportCurrentSceneToSkeletonAsset()
{
	if (_currentScene == nullptr) {
		SS_CLASS_WARNING_LOG("No scene to load");
		return;
	}

	FbxNode* skeletonRoot = FindSkeletonRootRecursion(_currentScene->GetRootNode());

	uint32 childCount = 0;
	if (skeletonRoot != nullptr)
	{
		childCount = FindFbxNodeWholeChildCnt(skeletonRoot);

		SSSkeletonAsset* newSkeleton = DBG_NEW SSSkeletonAsset(skeletonRoot->GetName(), childCount);
		ImportBoneToSkeletonRecursion(newSkeleton->_boneList, skeletonRoot, 65535);
		newSkeleton->_assetPath = _FBXImporter->GetFileName().Buffer();
		newSkeleton->_assetName = _fileName.C_Str();
		newSkeleton->_assetName += ".skl";
		SSSkeletonAssetManager::Get()->InsertNewAsset(newSkeleton);
	}

}

void SSFBXImporter::ImportCurrentSceneToSkeletonAnimAsset()
{
	FbxAnimStack* currAnimStack = _currentScene->GetCurrentAnimationStack();
	if (currAnimStack == nullptr) return;

	SS::FixedStringA<ASSET_NAME_LEN_MAX> assetName = _fileName;
	assetName += ".skl";
	SSSkeletonAsset* skeletonAsset = SSSkeletonAssetManager::FindAssetWithName(assetName);
	if (skeletonAsset == nullptr) return;

	assetName = _fileName;
	assetName += ".ska";



	FbxString animStackName = currAnimStack->GetName();
	FbxTakeInfo* takeInfo = _currentScene->GetTakeInfo(animStackName);
	FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
	FbxTime end = takeInfo->mLocalTimeSpan.GetStop();


	int64 frameStart = start.GetFrameCount(FbxTime::eFrames24);
	int64 frameEnd = end.GetFrameCount(FbxTime::eFrames24);
	int64 frameCnt = frameEnd - frameStart + 1;

	SSSkeletonAnimAsset* skeletonAnimAsset = DBG_NEW SSSkeletonAnimAsset(assetName, skeletonAsset, frameCnt);
	const SS::PooledList<BoneNode>& bones = skeletonAsset->GetBones();

	for (uint32 skeletonIdx = 0; skeletonIdx < bones.GetSize(); skeletonIdx++)
	{
		FbxNode* currentNode = _currentScene->FindNodeByName(bones[skeletonIdx]._boneName.C_Str());
		assert(currentNode);

		for (int64 frameIdx = frameStart; frameIdx <= frameEnd; ++frameIdx)
		{
			FbxTime currTime;
			currTime.SetFrame(frameIdx, FbxTime::eFrames24);
			Transform transform = ExtractTransformFromNode(currentNode, currTime);
			skeletonAnimAsset->SetAnimStackTransform(skeletonIdx, frameIdx - frameStart, transform);
		}
	}

	SSSkeletonAnimAssetManager::Get()->InsertNewAsset(skeletonAnimAsset);

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

			SSGeometryAsset* newGeometry = nullptr;

			if (fbxMesh->GetDeformerCount() == 0)
				newGeometry = GenerateGeometryFromFbxMesh(fbxMesh);
			else
				newGeometry = GenerateSkinnedGeometryFromFbxMesh(fbxMesh);

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
				{
					modelMaterial = SSMaterialAssetManager::GetEmptyAsset();
				}
				else if (newGeometry->GetMeshType() == EMeshType::Skinned)
				{
					modelMaterial->ChangeShader(SSShaderAssetManager::SSDefaultPbrSkinnedShaderName);
				}

				newModel->SetMaterial(modelMaterial, i);

			}
			if (matCnt == 0)
			{
				newModel->SetMaterial(SSMaterialAssetManager::GetEmptyAsset(), 0);
			}

			// model combination asset creation
			assetName = fbxMesh->GetNode()->GetName();
			SSModelCombinationAsset* newModelCombAsset;
			thisAsset = newModelCombAsset = DBG_NEW SSModelCombinationAsset(assetName, newModel, childCount);

			SSGeometryAssetManager::Get()->InsertNewGeometry(newGeometry);
			SSModelAssetManager::Get()->InsertNewModel(newModel);


			// HACK: Info print
			{

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

				if (fbxMesh->GetElementMaterial() != nullptr && fbxMesh->GetElementMaterial()->GetMappingMode() == FbxLayerElement::eByPolygon)
				{
					FbxLayerElementArrayTemplate<int>* materialIndices;
					fbxMesh->GetMaterialIndices(&materialIndices);

					SS_LOG("\t(By Polygon) material indice count: %d, polygon count: %d\n", materialIndices->GetCount(), fbxMesh->GetPolygonCount());
				}
				else
				{
					SS_LOG("\t(All Same)\n");
				}
				SS_LOG("\n\n");
			}
		}
		break;

		case FbxNodeAttribute::eSkeleton:
		{
			return;
		}
		default:
		{
			thisAsset = DBG_NEW SSPlaceableAsset(AssetType::Blank, childCount);
		}
		break;
		}



	}


	if (thisAsset == nullptr)
		thisAsset = DBG_NEW SSPlaceableAsset(AssetType::Blank, childCount);

	thisAsset->_transform = ExtractTransformFromNode(node);

	thisAsset->_parent = parentAsset;
	parentAsset->_childs.PushBack(thisAsset);
	for (uint32 i = 0; i < childCount; i++) {
		TraverseNodesRecursion(node->GetChild(i), thisAsset);
	}
}

int32 FindBoneIdxWithNameFromSkeleton(const SSSkeletonAsset* InAsset, const char* name)
{
	const SS::PooledList<BoneNode>& bones = InAsset->GetBones();

	for (uint32 i = 0; i < bones.GetSize(); i++)
	{
		if (strcmp(bones[i]._boneName, name) == 0) return i;
	}

	return -1;
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


	// * calculate vertex unit
	NewGeometryAsset->_vertexUnit = EVertexUnit::VertexPerPoint;
	if (FbxNormal->GetMappingMode() == FbxLayerElement::eByPolygonVertex)
		NewGeometryAsset->_vertexUnit = EVertexUnit::VertexPerPolygon;
	if (uvChannelCnt > 0 && fbxUV[0]->GetMappingMode() == FbxLayerElement::eByPolygonVertex)
		NewGeometryAsset->_vertexUnit = EVertexUnit::VertexPerPolygon;
	if (uvChannelCnt > 1 && fbxUV[1]->GetMappingMode() == FbxLayerElement::eByPolygonVertex)
		NewGeometryAsset->_vertexUnit = EVertexUnit::VertexPerPolygon;

	NewGeometryAsset->_drawTopologyType = EGeometryDrawTopology::TriangleList;
	NewGeometryAsset->_meshType = EMeshType::Rigid;

	// 2. alloc vertex memory
	switch (NewGeometryAsset->_vertexUnit) {

	case EVertexUnit::VertexPerPoint:
		NewGeometryAsset->_vertexCnt = ControlPointNum; break;
	case EVertexUnit::VertexPerPolygon:
		NewGeometryAsset->_vertexCnt = PolygonVertexNum; break;

	}


	NewGeometryAsset->_eachVertexDataSize = sizeof(SSDefaultVertex);
	NewGeometryAsset->_vertexData = DBG_MALLOC(NewGeometryAsset->_eachVertexDataSize * NewGeometryAsset->_vertexCnt);
	SSDefaultVertex* defaultVertices = (SSDefaultVertex*)NewGeometryAsset->_vertexData;

	// 3. alloc index memory
	if (fbxMesh->GetNode()->GetMaterial(0) != nullptr)
		NewGeometryAsset->_subGeometryNum = fbxMesh->GetNode()->GetMaterialCount();
	else
		NewGeometryAsset->_subGeometryNum = 1;
	assert(NewGeometryAsset->_subGeometryNum < SUBGEOM_COUNT_MAX);
	FbxLayerElementArrayTemplate<int>* materialIndices = nullptr;

	if (fbxMesh->GetElementMaterial() != nullptr && fbxMesh->GetElementMaterial()->GetMappingMode() == FbxLayerElement::eByPolygon)
	{
		fbxMesh->GetMaterialIndices(&materialIndices);

		for (uint32 i = 0; i < PolygonCount; i++)
		{
			uint8 matIdx = materialIndices->GetAt(i);
			NewGeometryAsset->_indexDataNum[matIdx] += ((fbxMesh->GetPolygonSize(i) - 2) * 3);
		}
	}
	else
	{
		for (uint32 i = 0; i < PolygonCount; i++)
		{
			NewGeometryAsset->_indexDataNum[0] += (fbxMesh->GetPolygonSize(i) - 2);
		}
		NewGeometryAsset->_indexDataNum[0] *= 3;
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
			defaultVertices[i].Pos.X = -ControlPoints[i].mData[0] * 0.01;
			defaultVertices[i].Pos.Y = ControlPoints[i].mData[1] * 0.01;
			defaultVertices[i].Pos.Z = ControlPoints[i].mData[2] * 0.01;
			defaultVertices[i].Pos.W = ControlPoints[i].mData[3] * 0.01;
		}

		if (fbxMesh->GetElementMaterial() != nullptr && fbxMesh->GetElementMaterial()->GetMappingMode() == FbxLayerElement::eByPolygon)
		{
			for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++)
			{
				uint32 thisPolygonSize = fbxMesh->GetPolygonSize(i);
				uint32 matIdx = materialIndices->GetAt(i);
				uint32 idxDataStart = NewGeometryAsset->_indexDataStartIndex[matIdx];
				for (uint32 j = 1; j < thisPolygonSize - 1; j++)
				{
					NewGeometryAsset->_indexData[idxDataStart + CurIndexDataIdx[matIdx]] = fbxMesh->GetPolygonVertex(i, j + 1);
					NewGeometryAsset->_indexData[idxDataStart + CurIndexDataIdx[matIdx] + 1] = fbxMesh->GetPolygonVertex(i, j);
					NewGeometryAsset->_indexData[idxDataStart + CurIndexDataIdx[matIdx] + 2] = fbxMesh->GetPolygonVertex(i, 0);

					CurIndexDataIdx[matIdx] += 3;
				}
			}
		}
		else
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
				defaultVertices[VertexIdxCounter].Pos.X = -vertexPos.mData[0] * 0.01;
				defaultVertices[VertexIdxCounter].Pos.Y = vertexPos.mData[1] * 0.01;
				defaultVertices[VertexIdxCounter].Pos.Z = vertexPos.mData[2] * 0.01;
				defaultVertices[VertexIdxCounter].Pos.W = vertexPos.mData[3] * 0.01;

				VertexIdxCounter++;
			}
		}
		assert(VertexIdxCounter == PolygonVertexNum);


		if (fbxMesh->GetElementMaterial() != nullptr && fbxMesh->GetElementMaterial()->GetMappingMode() == FbxLayerElement::eByPolygon)
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
		else
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

				defaultVertices[i].Normal.X = -normalVector.mData[0];
				defaultVertices[i].Normal.Y = normalVector.mData[1];
				defaultVertices[i].Normal.Z = normalVector.mData[2];
				defaultVertices[i].Normal.W = normalVector.mData[3];
				defaultVertices[i].Normal = defaultVertices[i].Normal.Get3DNormalized();
			}

			break;
		case FbxLayerElement::eIndex:
			SS_CLASS_WARNING_LOG("\"FbxLayerElement::EReferenceMode::eIndex\" is Legacy. Use \"eIndexToDirect\".");
		case FbxLayerElement::eIndexToDirect:

			for (int32 i = 0; i < fbxMesh->GetControlPointsCount(); i++) {
				int32 FbxNormalIdx = FbxNormal->GetIndexArray().GetAt(i);
				FbxVector4 normalVector = FbxNormal->GetDirectArray().GetAt(FbxNormalIdx);

				defaultVertices[i].Normal.X = -normalVector.mData[0];
				defaultVertices[i].Normal.Y = normalVector.mData[1];
				defaultVertices[i].Normal.Z = normalVector.mData[2];
				defaultVertices[i].Normal.W = normalVector.mData[3];
				defaultVertices[i].Normal = defaultVertices[i].Normal.Get3DNormalized();
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
					defaultVertices[VertexIdxCounter].Normal.X = -normalVector.mData[0];
					defaultVertices[VertexIdxCounter].Normal.Y = normalVector.mData[1];
					defaultVertices[VertexIdxCounter].Normal.Z = normalVector.mData[2];
					defaultVertices[VertexIdxCounter].Normal.W = normalVector.mData[3];
					defaultVertices[VertexIdxCounter].Normal = defaultVertices[VertexIdxCounter].Normal.Get3DNormalized();

					VertexIdxCounter++;
				}
			}
			assert(VertexIdxCounter == NewGeometryAsset->_vertexCnt);

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

					defaultVertices[VertexIdxCounter].Normal.X = -normalVector.mData[0];
					defaultVertices[VertexIdxCounter].Normal.Y = normalVector.mData[1];
					defaultVertices[VertexIdxCounter].Normal.Z = normalVector.mData[2];
					defaultVertices[VertexIdxCounter].Normal.W = normalVector.mData[3];
					defaultVertices[VertexIdxCounter].Normal = defaultVertices[VertexIdxCounter].Normal.Get3DNormalized();

					VertexIdxCounter++;
				}
			}
			assert(VertexIdxCounter == NewGeometryAsset->_vertexCnt);
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

					defaultVertices[i].Uv[uvIdx].X = uvVector.mData[0];
					defaultVertices[i].Uv[uvIdx].Y = 1 - uvVector.mData[1];
				}

				break;
			case FbxLayerElement::eIndex:
				SS_CLASS_WARNING_LOG("\"FbxLayerElement::EReferenceMode::eIndex\" is Legacy. Use \"eIndexToDirect\".");
			case FbxLayerElement::eIndexToDirect:

				for (int32 i = 0; i < fbxMesh->GetControlPointsCount(); i++) {
					int32 FbxUvIdx = fbxUVItem->GetIndexArray().GetAt(i);
					FbxVector2 uvVector = fbxUVItem->GetDirectArray().GetAt(FbxUvIdx);

					defaultVertices[i].Uv[uvIdx].X = uvVector.mData[0];
					defaultVertices[i].Uv[uvIdx].Y = 1 - uvVector.mData[1];
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

						defaultVertices[VertexIdxCounter].Uv[uvIdx].X = uvVector.mData[0];
						defaultVertices[VertexIdxCounter].Uv[uvIdx].Y = 1 - uvVector.mData[1];

						VertexIdxCounter++;
					}
				}
				assert(VertexIdxCounter == NewGeometryAsset->_vertexCnt);

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

						defaultVertices[VertexIdxCounter].Uv[uvIdx].X = uvVector.mData[0];
						defaultVertices[VertexIdxCounter].Uv[uvIdx].Y = 1 - uvVector.mData[1];

						VertexIdxCounter++;
					}
				}
				assert(VertexIdxCounter == NewGeometryAsset->_vertexCnt);

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

					defaultVertices[i].Tangent.X = tanVector.mData[0];
					defaultVertices[i].Tangent.Y = tanVector.mData[1];
					defaultVertices[i].Tangent.Z = tanVector.mData[2];
					defaultVertices[i].Tangent.W = tanVector.mData[3];
					defaultVertices[i].Tangent = defaultVertices[i].Tangent.Get3DNormalized();
				}
				break;
			case FbxLayerElement::eIndex:
				SS_CLASS_WARNING_LOG("\"FbxLayerElement::EReferenceMode::eIndex\" is Legacy. Use \"eIndexToDirect\".");
			case FbxLayerElement::eIndexToDirect:
				for (int32 i = 0; i < fbxMesh->GetControlPointsCount(); i++) {
					int32 tangentIdx = fbxTangent->GetIndexArray().GetAt(i);
					FbxVector4 tanVector = fbxTangent->GetDirectArray().GetAt(tangentIdx);

					defaultVertices[i].Tangent.X = -tanVector.mData[0];
					defaultVertices[i].Tangent.Y = tanVector.mData[1];
					defaultVertices[i].Tangent.Z = tanVector.mData[2];
					defaultVertices[i].Tangent.W = tanVector.mData[3];
					defaultVertices[i].Tangent = defaultVertices[i].Tangent.Get3DNormalized();
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
						defaultVertices[VertexIdxCounter].Tangent.X = -tanVector.mData[0];
						defaultVertices[VertexIdxCounter].Tangent.Y = tanVector.mData[1];
						defaultVertices[VertexIdxCounter].Tangent.Z = tanVector.mData[2];
						defaultVertices[VertexIdxCounter].Tangent.W = tanVector.mData[3];
						defaultVertices[i].Tangent = defaultVertices[i].Tangent.Get3DNormalized();

						VertexIdxCounter++;
					}
				}
				assert(VertexIdxCounter == NewGeometryAsset->_vertexCnt);
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
						defaultVertices[VertexIdxCounter].Tangent.X = -tanVector.mData[0];
						defaultVertices[VertexIdxCounter].Tangent.Y = tanVector.mData[1];
						defaultVertices[VertexIdxCounter].Tangent.Z = tanVector.mData[2];
						defaultVertices[VertexIdxCounter].Tangent.W = tanVector.mData[3];
						defaultVertices[i].Tangent = defaultVertices[i].Tangent.Get3DNormalized();

						VertexIdxCounter++;
					}
				}
				assert(VertexIdxCounter == NewGeometryAsset->_vertexCnt);
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
				SSDefaultVertex& v0 = defaultVertices[thisIdxData[i]];
				SSDefaultVertex& v1 = defaultVertices[thisIdxData[i + 1]];
				SSDefaultVertex& v2 = defaultVertices[thisIdxData[i + 2]];

				Vector4f dv1 = v1.Pos - v0.Pos;
				Vector4f dv2 = v2.Pos - v0.Pos;

				Vector2f duv1 = v1.Uv[0] - v0.Uv[0];
				Vector2f duv2 = v2.Uv[0] - v0.Uv[0];

				float detInverse = 1.0f / (duv1.X * duv2.Y - duv1.Y * duv2.X);
				Vector4f tangent = (dv1 * duv2.Y - dv2 * duv1.Y) * detInverse;
				v2.Tangent = v1.Tangent = v0.Tangent = tangent;
			}
		}
	}

	NewGeometryAsset->_assetPath = _FBXImporter->GetFileName().Buffer();
	NewGeometryAsset->_assetName = _fileName.C_Str();
	NewGeometryAsset->_assetName += "_";
	NewGeometryAsset->_assetName += fbxMesh->GetNode()->GetName();
	NewGeometryAsset->_assetName += ".geom";

	return NewGeometryAsset;
}

SSDefaultVertex ExtractVertex(FbxMesh* fbxMesh, uint32 polygonIdx, uint32 positionInPolygon, uint32& outControlPointIdx)
{
	SSDefaultVertex outVertex;
	outControlPointIdx = fbxMesh->GetPolygonVertex(polygonIdx, positionInPolygon);


	// Position
	FbxVector4 controlPointPosition = fbxMesh->GetControlPointAt(outControlPointIdx);
	outVertex.Pos.X = -controlPointPosition.mData[0] * 0.01;
	outVertex.Pos.Y = controlPointPosition.mData[1] * 0.01;
	outVertex.Pos.Z = controlPointPosition.mData[2] * 0.01;
	outVertex.Pos.W = controlPointPosition.mData[3] * 0.01;


	// Normal
	const FbxGeometryElementNormal* const fbxNormal = fbxMesh->GetElementNormal();
	FbxVector4 normalVector;
	int32 fbxNormalIdx;
	switch (fbxNormal->GetReferenceMode())
	{
	case FbxLayerElement::eDirect:

		normalVector = fbxNormal->GetDirectArray().GetAt(outControlPointIdx);

		break;
	case FbxLayerElement::eIndex:
	case FbxLayerElement::eIndexToDirect:

		fbxNormalIdx = fbxNormal->GetIndexArray().GetAt(outControlPointIdx);
		normalVector = fbxNormal->GetDirectArray().GetAt(fbxNormalIdx);

		break;
	default:
		WASSERT_WITH_MESSAGE(false, "Invalid Fbxformat. function just return");
	}

	normalVector.Normalize();
	outVertex.Normal.X = normalVector.mData[0];
	outVertex.Normal.Y = normalVector.mData[1];
	outVertex.Normal.Z = normalVector.mData[2];
	outVertex.Normal.W = 0;


	// UV
	uint32 uvChannelCnt = fbxMesh->GetUVLayerCount();
	if (uvChannelCnt > 2)
	{
		WASSERT_WITH_MESSAGE(false, "Too many uv channel");
		uvChannelCnt = 2;
	}

	for (uint32 i = 0; i < uvChannelCnt; i++)
	{
		FbxVector2 uvVector;
		uint32 uvIdx;
		uint32 polygonVertexIdx;
		uint32 directIdx;
		FbxGeometryElementUV* fbxUV = fbxMesh->GetElementUV(i);
		assert(fbxUV != nullptr);

		switch (fbxUV->GetMappingMode())
		{
		case FbxLayerElement::eByControlPoint:

			switch (fbxUV->GetReferenceMode())
			{
			case FbxLayerElement::eDirect:

				uvVector = fbxUV->GetDirectArray().GetAt(outControlPointIdx);

				break;
			case FbxLayerElement::eIndex:
			case FbxLayerElement::eIndexToDirect:

				uvIdx = fbxUV->GetIndexArray().GetAt(outControlPointIdx);
				uvVector = fbxUV->GetDirectArray().GetAt(uvIdx);

				break;
			default:
				WASSERT_WITH_MESSAGE(false, "Invalid Fbxformat. function just return");
				break;
			}

			break;


		case FbxLayerElement::eByPolygonVertex:
			// uv���� ��쿡�� Ư���ϰ� eByPolygonVertex�� �� 
			// reference mode�� �����ϰ� GetTextureUVIndex�� ���� �ٷ� directarray�� index�� ���;� �Ѵ�.

			uvIdx = fbxMesh->GetTextureUVIndex(polygonIdx, positionInPolygon);
			uvVector = fbxUV->GetDirectArray().GetAt(uvIdx);

			break;
		}

		outVertex.Uv[i].X = uvVector.mData[0];
		outVertex.Uv[i].Y = 1 - uvVector.mData[1];

		break;
	}




	// Tangent
	FbxGeometryElementTangent* fbxTangent = fbxMesh->GetElementTangent();
	FbxVector4 tanVector;
	if (fbxTangent != nullptr) {

		switch (fbxTangent->GetReferenceMode())
		{
		case FbxLayerElement::eDirect:

			tanVector = fbxTangent->GetDirectArray().GetAt(outControlPointIdx);

			break;
		case FbxLayerElement::eIndex:
		case FbxLayerElement::eIndexToDirect:

			int32 tangentIdx = fbxTangent->GetIndexArray().GetAt(outControlPointIdx);
			tanVector = fbxTangent->GetDirectArray().GetAt(tangentIdx);

			break;
		}

		outVertex.Tangent.X = tanVector.mData[0];
		outVertex.Tangent.Y = tanVector.mData[1];
		outVertex.Tangent.Z = tanVector.mData[2];
		outVertex.Tangent.W = tanVector.mData[3];
	}
	else
	{
		outVertex.Tangent.X = 0.0f;
		outVertex.Tangent.Y = 0.0f;
		outVertex.Tangent.Z = 0.0f;
		outVertex.Tangent.W = 0.0f;
	}

	return outVertex;
}

SSSkinnedVertex ExtractSkinnedVertexWithoutSkinData(FbxMesh* fbxMesh, uint32 polygonIdx, uint32 positionInPolygon, uint32& outControlPointIdx)
{
	SSSkinnedVertex outVertex;
	outControlPointIdx = fbxMesh->GetPolygonVertex(polygonIdx, positionInPolygon);


	// Position
	FbxVector4 controlPointPosition = fbxMesh->GetControlPointAt(outControlPointIdx);
	outVertex.Pos.X = -controlPointPosition.mData[0] * 0.01;
	outVertex.Pos.Y = controlPointPosition.mData[1] * 0.01;
	outVertex.Pos.Z = controlPointPosition.mData[2] * 0.01;
	outVertex.Pos.W = controlPointPosition.mData[3] * 0.01;


	// Normal
	const FbxGeometryElementNormal* const fbxNormal = fbxMesh->GetElementNormal();
	FbxVector4 normalVector;
	int32 fbxNormalIdx;
	switch (fbxNormal->GetReferenceMode())
	{
	case FbxLayerElement::eDirect:

		normalVector = fbxNormal->GetDirectArray().GetAt(outControlPointIdx);

		break;
	case FbxLayerElement::eIndex:
	case FbxLayerElement::eIndexToDirect:

		fbxNormalIdx = fbxNormal->GetIndexArray().GetAt(outControlPointIdx);
		normalVector = fbxNormal->GetDirectArray().GetAt(fbxNormalIdx);

		break;
	default:
		WASSERT_WITH_MESSAGE(false, "Invalid Fbxformat. function just return");
	}

	normalVector.Normalize();
	outVertex.Normal.X = -normalVector.mData[0];
	outVertex.Normal.Y = normalVector.mData[1];
	outVertex.Normal.Z = normalVector.mData[2];
	outVertex.Normal.W = normalVector.mData[3];


	// UV
	uint32 uvChannelCnt = fbxMesh->GetUVLayerCount();
	if (uvChannelCnt > 2)
	{
		WASSERT_WITH_MESSAGE(false, "Too many uv channel");
		uvChannelCnt = 2;
	}

	for (uint32 i = 0; i < uvChannelCnt; i++)
	{
		FbxVector2 uvVector;
		uint32 uvIdx;
		uint32 polygonVertexIdx;
		uint32 directIdx;
		FbxGeometryElementUV* fbxUV = fbxMesh->GetElementUV(i);
		assert(fbxUV != nullptr);

		switch (fbxUV->GetMappingMode())
		{
		case FbxLayerElement::eByControlPoint:

			switch (fbxUV->GetReferenceMode())
			{
			case FbxLayerElement::eDirect:

				uvVector = fbxUV->GetDirectArray().GetAt(outControlPointIdx);

				break;
			case FbxLayerElement::eIndex:
			case FbxLayerElement::eIndexToDirect:

				uvIdx = fbxUV->GetIndexArray().GetAt(outControlPointIdx);
				uvVector = fbxUV->GetDirectArray().GetAt(uvIdx);

				break;
			default:
				WASSERT_WITH_MESSAGE(false, "Invalid Fbxformat. function just return");
				break;
			}

			break;


		case FbxLayerElement::eByPolygonVertex:
			// uv���� ��쿡�� Ư���ϰ� eByPolygonVertex�� �� 
			// reference mode�� �����ϰ� GetTextureUVIndex�� ���� �ٷ� directarray�� index�� ���;� �Ѵ�.

			uvIdx = fbxMesh->GetTextureUVIndex(polygonIdx, positionInPolygon);
			uvVector = fbxUV->GetDirectArray().GetAt(uvIdx);

			break;
		}

		outVertex.Uv[i].X = uvVector.mData[0];
		outVertex.Uv[i].Y = 1 - uvVector.mData[1];

		break;
	}



	// Tangent
	FbxGeometryElementTangent* fbxTangent = fbxMesh->GetElementTangent();
	FbxVector4 tanVector;
	if (fbxTangent != nullptr) {

		switch (fbxTangent->GetReferenceMode())
		{
		case FbxLayerElement::eDirect:

			tanVector = fbxTangent->GetDirectArray().GetAt(outControlPointIdx);

			break;
		case FbxLayerElement::eIndex:
		case FbxLayerElement::eIndexToDirect:

			int32 tangentIdx = fbxTangent->GetIndexArray().GetAt(outControlPointIdx);
			tanVector = fbxTangent->GetDirectArray().GetAt(tangentIdx);

			break;
		}

		outVertex.Tangent.X = -tanVector.mData[0];
		outVertex.Tangent.Y = tanVector.mData[1];
		outVertex.Tangent.Z = tanVector.mData[2];
		outVertex.Tangent.W = tanVector.mData[3];
	}
	else
	{
		outVertex.Tangent.X = 0.0f;
		outVertex.Tangent.Y = 0.0f;
		outVertex.Tangent.Z = 0.0f;
		outVertex.Tangent.W = 0.0f;
	}


	// Skinning
	outVertex.BoneIdx[0] = SS_UINT32_MAX;
	outVertex.BoneIdx[1] = SS_UINT32_MAX;
	outVertex.BoneIdx[2] = SS_UINT32_MAX;
	outVertex.BoneIdx[3] = SS_UINT32_MAX;
	outVertex.Weight[0] = 0;
	outVertex.Weight[1] = 0;
	outVertex.Weight[2] = 0;
	outVertex.Weight[3] = 0;


	return outVertex;
}

constexpr float POS_SQR_THRESHOLD = 0.0001;
constexpr float DEG_COS_THRESHOLD = 0.001;
constexpr float UV_DIST_THRESHOlD = 0.0001;
constexpr float FLOAT_ZERO_THRESHOLD = 0.0001;
bool AreSimilarVertex(const SSDefaultVertex& lhs, const SSDefaultVertex& rhs)
{
	if (SS::SqrDistance(lhs.Pos, rhs.Pos) > POS_SQR_THRESHOLD)
		return false;

	if (SS::SqrDistance(rhs.Normal, lhs.Normal) > FLOAT_ZERO_THRESHOLD &&
		SS::abs(1 - SS::Dot3D(lhs.Normal, rhs.Normal)) > DEG_COS_THRESHOLD)
		return false;

	if (SS::SqrDistance(rhs.Tangent, lhs.Tangent) > FLOAT_ZERO_THRESHOLD &&
		SS::abs(1 - SS::Dot3D(lhs.Tangent, rhs.Tangent)) > DEG_COS_THRESHOLD)
		return false;

	if (SS::SqrDistance(lhs.Uv[0], rhs.Uv[0]) > UV_DIST_THRESHOlD)
		return false;

	if (SS::SqrDistance(lhs.Uv[1], rhs.Uv[1]) > UV_DIST_THRESHOlD)
		return false;

	return true;
}

SSGeometryAsset* SSFBXImporter::GenerateSkinnedGeometryFromFbxMesh(FbxMesh* fbxMesh)
{
	assert(fbxMesh != nullptr);
	SSGeometryAsset* NewGeometryAsset = DBG_NEW SSGeometryAsset();


	// 1. Load num
	const uint32 layerNum = fbxMesh->GetLayerCount();
	const uint32 ControlPointCnt = fbxMesh->GetControlPointsCount();
	const uint32 PolygonCount = fbxMesh->GetPolygonCount();
	const uint32 PolygonVertexCnt = fbxMesh->GetPolygonVertexCount(); // sum of vertex in each polygon
	const FbxGeometryElementNormal* const FbxNormal = fbxMesh->GetElementNormal();
	SS_ASSERT(FbxNormal != nullptr, "normal must be exists.");


	SS::PooledList<SS::PooledList<uint32>> ControlPointToSSIdxMap(ControlPointCnt);
	ControlPointToSSIdxMap.Resize(ControlPointCnt);
	for (SS::PooledList<uint32>& item : ControlPointToSSIdxMap)
	{
		item.IncreaseCapacityAndCopy(10);
	}

	SS::PooledList<SS::PooledList<SS::pair<uint32, int32>>> PolygonVertexToCtrlPointMap;

	uint32 uvChannelCnt = fbxMesh->GetUVLayerCount();
	FbxGeometryElementUV* fbxUV[2];
	if (uvChannelCnt > 2)
	{
		SS_CLASS_ERR_LOG("Too many uv channel");
		uvChannelCnt = 2;
	}
	for (uint32 i = 0; i < uvChannelCnt; i++)
	{
		fbxUV[i] = fbxMesh->GetElementUV(i);
		SS_ASSERT(fbxUV[i] != nullptr, "uv must be exists of idx %d", i);
	}


	// * calculate vertex unit
	NewGeometryAsset->_vertexUnit = EVertexUnit::VertexPerPoint;
	if (FbxNormal->GetMappingMode() == FbxLayerElement::eByPolygonVertex)
		NewGeometryAsset->_vertexUnit = EVertexUnit::VertexPerPolygon;
	if (uvChannelCnt > 0 && fbxUV[0]->GetMappingMode() == FbxLayerElement::eByPolygonVertex)
		NewGeometryAsset->_vertexUnit = EVertexUnit::VertexPerPolygon;
	if (uvChannelCnt > 1 && fbxUV[1]->GetMappingMode() == FbxLayerElement::eByPolygonVertex)
		NewGeometryAsset->_vertexUnit = EVertexUnit::VertexPerPolygon;

	NewGeometryAsset->_drawTopologyType = EGeometryDrawTopology::TriangleList;
	NewGeometryAsset->_meshType = EMeshType::Skinned;



	// ======================================================= labatory
	PolygonVertexToCtrlPointMap.IncreaseCapacityAndCopy(PolygonCount);
	PolygonVertexToCtrlPointMap.Resize(PolygonCount);
	SS::PooledList<SSSkinnedVertex> ssVertexBuffer(ControlPointCnt * 2);

	for (uint32 i = 0; i < PolygonCount; i++)
	{
		uint32 PolygonVertexCount = fbxMesh->GetPolygonSize(i);
		PolygonVertexToCtrlPointMap[i].IncreaseCapacityAndCopy(PolygonVertexCount);
		for (uint32 j = 0; j < PolygonVertexCount; j++)
		{
			uint32 ControlPointIdx;
			SSSkinnedVertex extractedVertex = ExtractSkinnedVertexWithoutSkinData(fbxMesh, i, j, ControlPointIdx);
			SS_ASSERT(ControlPointIdx != -1, "Invalid ControlPoint");

			int32 ctrlPointListIdx = -1;
			for (uint32 i = 0; i < ControlPointToSSIdxMap[ControlPointIdx].GetSize(); i++)
			{
				uint32 ssIdx = ControlPointToSSIdxMap[ControlPointIdx][i];
				if (AreSimilarVertex(ssVertexBuffer[ssIdx], extractedVertex))
				{
					ctrlPointListIdx = i;
					break;
				}
			}

			if (ctrlPointListIdx == -1)
			{
				PolygonVertexToCtrlPointMap[i].PushBack({ ControlPointIdx, (int32)ControlPointToSSIdxMap[ControlPointIdx].GetSize() });
				ControlPointToSSIdxMap[ControlPointIdx].PushBackCapacity(ssVertexBuffer.GetSize());
				ssVertexBuffer.PushBackCapacity(extractedVertex);
			}
			else
			{
				PolygonVertexToCtrlPointMap[i].PushBack({ ControlPointIdx, ctrlPointListIdx });
			}
		}
	}


	uint32 ssVertexCnt = 0;
	for (uint32 i = 0; i < ControlPointToSSIdxMap.GetSize(); i++)
	{
		ssVertexCnt += ControlPointToSSIdxMap[i].GetSize();
	}


	// Load Skinning Data
	SS::PooledList<uint8> boneCntArr(ControlPointCnt);
	boneCntArr.Resize(ControlPointCnt);
	for (uint32 i = 0; i < ControlPointCnt; i++) boneCntArr[i] = 0;


	uint32 deformerCnt = fbxMesh->GetDeformerCount();
	assert(deformerCnt == 1);

	FbxSkin* fbxSkin = static_cast<FbxSkin*>(fbxMesh->GetDeformer(0, FbxDeformer::eSkin));
	assert(fbxSkin != nullptr);
	uint32 clusterCnt = fbxSkin->GetClusterCount();

	SS::FixedStringA<ASSET_NAME_LEN_MAX> skeletonName = _fileName;
	skeletonName += ".skl";
	SSSkeletonAsset* skeleton = SSSkeletonAssetManager::FindAssetWithName(skeletonName);
	assert(skeleton != nullptr);
	NewGeometryAsset->boundSkeletonAsset = skeleton;

	for (uint32 i = 0; i < clusterCnt; i++)
	{
		FbxCluster* curCluster = fbxSkin->GetCluster(i);
		int32 jointIdx = FindBoneIdxWithNameFromSkeleton(skeleton, curCluster->GetLink()->GetName());

		if (jointIdx == -1)
		{
			SS_CLASS_ERR_LOG("Skinned mesh doesn't match with skeleton.");
		}

		uint32 clusterIndicesCnt = curCluster->GetControlPointIndicesCount();
		int* curClusterCtlrPointIndices = curCluster->GetControlPointIndices();
		double* curClusterCtrlPointWeights = curCluster->GetControlPointWeights();

		for (uint32 j = 0; j < clusterIndicesCnt; j++)
		{
			int ctrlPointIdx = curClusterCtlrPointIndices[j];
			double ctrlPointWeight = curClusterCtrlPointWeights[j];
			uint32 boneCnt = boneCntArr[ctrlPointIdx];

			constexpr float SKIN_WEIGHT_THRESHOLD = 0.03;
			if (ctrlPointWeight <= SKIN_WEIGHT_THRESHOLD)
			{
				for (uint32 ssIdx : ControlPointToSSIdxMap[ctrlPointIdx])
				{
					ssVertexBuffer[ssIdx].Weight[boneCnt] += ctrlPointWeight;
				}
				continue;
			}

			for (uint32 ssIdx : ControlPointToSSIdxMap[ctrlPointIdx])
			{
				ssVertexBuffer[ssIdx].BoneIdx[boneCnt] = jointIdx;
				ssVertexBuffer[ssIdx].Weight[boneCnt] += ctrlPointWeight;
			}
			boneCntArr[ctrlPointIdx]++;
		}
	}

	for (uint32 i = 0; i < ControlPointCnt; i++) assert(boneCntArr[i] <= 4);


	// For metric
	uint32 minIdx = 0;
	float minPosY = 0;
	uint32 maxIdx = 0;
	float maxPosY = 0;
	for (uint32 i = 0; i < ssVertexBuffer.GetSize(); i++)
	{
		if (ssVertexBuffer[i].Pos.Y < minPosY)
		{
			minIdx = i;
			minPosY = ssVertexBuffer[i].Pos.Y;
		}

		if (ssVertexBuffer[i].Pos.Y > maxPosY)
		{
			maxIdx = i;
			maxPosY = ssVertexBuffer[i].Pos.Y;
		}
	}
	// For metric

	// ================================================================


	// 2. alloc vertex memory
	NewGeometryAsset->_vertexCnt = ssVertexBuffer.GetSize();
	NewGeometryAsset->_eachVertexDataSize = sizeof(SSSkinnedVertex);
	uint32 validVertexBufferSize = NewGeometryAsset->_eachVertexDataSize * NewGeometryAsset->_vertexCnt;
	NewGeometryAsset->_vertexData = DBG_MALLOC(validVertexBufferSize);
	SSSkinnedVertex* ssSkinnedVertex = (SSSkinnedVertex*)NewGeometryAsset->_vertexData;

	// 3. copy to real time vertex buffer
	memcpy_s(ssSkinnedVertex, validVertexBufferSize, ssVertexBuffer.GetData(), validVertexBufferSize);


	// 4. alloc index memory
	if (fbxMesh->GetNode()->GetMaterial(0) != nullptr)
		NewGeometryAsset->_subGeometryNum = fbxMesh->GetNode()->GetMaterialCount();
	else
		NewGeometryAsset->_subGeometryNum = 1;
	assert(NewGeometryAsset->_subGeometryNum < SUBGEOM_COUNT_MAX);

	FbxGeometryElementMaterial* fbxElementMaterial = fbxMesh->GetElementMaterial();

	FbxLayerElementArrayTemplate<int>* materialIndices = nullptr;
	if (fbxMesh->GetElementMaterial() != nullptr && fbxMesh->GetElementMaterial()->GetMappingMode() == FbxLayerElement::eByPolygon)
	{
		fbxMesh->GetMaterialIndices(&materialIndices);

		for (uint32 i = 0; i < PolygonCount; i++)
		{
			uint8 matIdx = materialIndices->GetAt(i);
			NewGeometryAsset->_indexDataNum[matIdx] += ((fbxMesh->GetPolygonSize(i) - 2) * 3);
		}
	}
	else
	{
		for (uint32 i = 0; i < PolygonCount; i++)
		{
			NewGeometryAsset->_indexDataNum[0] += (fbxMesh->GetPolygonSize(i) - 2);
		}
		NewGeometryAsset->_indexDataNum[0] *= 3;
	}

	uint32 idxAcc = 0;
	for (uint32 i = 0; i < NewGeometryAsset->_subGeometryNum; i++)
	{
		NewGeometryAsset->_indexDataStartIndex[i] = idxAcc;
		idxAcc += NewGeometryAsset->_indexDataNum[i];
	}
	NewGeometryAsset->_wholeIndexDataNum = idxAcc;
	NewGeometryAsset->_indexData = (uint32*)DBG_MALLOC(sizeof(uint32) * NewGeometryAsset->_wholeIndexDataNum);


	// 5. load index memory
	uint32 subMaterialIdxDataCounter[SUBGEOM_COUNT_MAX] = { 0, };

	if (fbxMesh->GetElementMaterial() != nullptr && fbxMesh->GetElementMaterial()->GetMappingMode() == FbxLayerElement::eByPolygon)
	{
		for (uint32 i = 0; i < PolygonCount; i++)
		{
			uint32 PolygonVertexCount = fbxMesh->GetPolygonSize(i);
			uint32 matIdx = materialIndices->GetAt(i);
			uint32 idxDataStart = NewGeometryAsset->_indexDataStartIndex[matIdx];
			for (uint32 j = 1; j < PolygonVertexCount - 1; j++)
			{
				SS::pair<uint32, int32> CtrlPointIdx = PolygonVertexToCtrlPointMap[i][0];
				uint32 ssIdx = ControlPointToSSIdxMap[CtrlPointIdx.first][CtrlPointIdx.second];
				NewGeometryAsset->_indexData[idxDataStart + subMaterialIdxDataCounter[matIdx]] = ssIdx;

				CtrlPointIdx = PolygonVertexToCtrlPointMap[i][j];
				ssIdx = ControlPointToSSIdxMap[CtrlPointIdx.first][CtrlPointIdx.second];
				NewGeometryAsset->_indexData[idxDataStart + subMaterialIdxDataCounter[matIdx] + 1] = ssIdx;

				CtrlPointIdx = PolygonVertexToCtrlPointMap[i][j + 1];
				ssIdx = ControlPointToSSIdxMap[CtrlPointIdx.first][CtrlPointIdx.second];
				NewGeometryAsset->_indexData[idxDataStart + subMaterialIdxDataCounter[matIdx] + 2] = ssIdx;

				subMaterialIdxDataCounter[matIdx] += 3;
			}
		}
	}
	else
	{
		for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++)
		{
			uint32 thisPolygonSize = fbxMesh->GetPolygonSize(i);
			for (uint32 j = 1; j < thisPolygonSize - 1; j++)
			{
				SS::pair<uint32, int32> CtrlPointIdx = PolygonVertexToCtrlPointMap[i][j + 1];
				uint32 ssIdx = ControlPointToSSIdxMap[CtrlPointIdx.first][CtrlPointIdx.second];
				NewGeometryAsset->_indexData[subMaterialIdxDataCounter[0]] = ssIdx;

				CtrlPointIdx = PolygonVertexToCtrlPointMap[i][j];
				ssIdx = ControlPointToSSIdxMap[CtrlPointIdx.first][CtrlPointIdx.second];
				NewGeometryAsset->_indexData[subMaterialIdxDataCounter[0] + 1] = ssIdx;

				CtrlPointIdx = PolygonVertexToCtrlPointMap[i][0];
				ssIdx = ControlPointToSSIdxMap[CtrlPointIdx.first][CtrlPointIdx.second];
				NewGeometryAsset->_indexData[subMaterialIdxDataCounter[0] + 2] = ssIdx;

				subMaterialIdxDataCounter[0] += 3;
			}
		}
	}

	for (uint32 i = 0; i < NewGeometryAsset->_subGeometryNum; i++)
		assert(NewGeometryAsset->_indexDataNum[i] == subMaterialIdxDataCounter[i]);



	// Load Tangent Data
	FbxGeometryElementTangent* fbxTangent = fbxMesh->GetElementTangent();
	if (fbxTangent == nullptr)
	{
		for (uint32 subGeomIdx = 0; subGeomIdx < NewGeometryAsset->_subGeometryNum; subGeomIdx++)
		{
			uint32* thisIdxData = NewGeometryAsset->_indexData + NewGeometryAsset->_indexDataStartIndex[subGeomIdx];
			uint32 thisIdxDataNum = NewGeometryAsset->_indexDataNum[subGeomIdx];
			assert(thisIdxDataNum % 3 == 0);

			for (uint32 i = 0; i < thisIdxDataNum; i += 3)
			{
				SSDefaultVertex& v0 = ssSkinnedVertex[thisIdxData[i]];
				SSDefaultVertex& v1 = ssSkinnedVertex[thisIdxData[i + 1]];
				SSDefaultVertex& v2 = ssSkinnedVertex[thisIdxData[i + 2]];

				Vector4f dv1 = v1.Pos - v0.Pos;
				Vector4f dv2 = v2.Pos - v0.Pos;

				Vector2f duv1 = v1.Uv[0] - v0.Uv[0];
				Vector2f duv2 = v2.Uv[0] - v0.Uv[0];

				float detInverse = 1.0f / (duv1.X * duv2.Y - duv1.Y * duv2.X);
				Vector4f tangent = (dv1 * duv2.Y - dv2 * duv1.Y) * detInverse;
				v2.Tangent = v1.Tangent = v0.Tangent = tangent;
			}
		}
	}

	NewGeometryAsset->_assetPath = _FBXImporter->GetFileName().Buffer();
	NewGeometryAsset->_assetName = _fileName.C_Str();
	NewGeometryAsset->_assetName += "_";
	NewGeometryAsset->_assetName += fbxMesh->GetNode()->GetName();
	NewGeometryAsset->_assetName += ".geom";

	return NewGeometryAsset;
}
