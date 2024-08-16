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

	const FbxQuaternion fbxRotation = fbxMat.GetQ(); // pitch yaw roll 으로 바꿔줘야 함
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

	SSSkeletonAnimAsset* skeletonAnimAsset = DBG_NEW SSSkeletonAnimAsset(assetName, skeletonAsset, frameCnt, 24);
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

	if (fbxNormal->GetMappingMode() == FbxLayerElement::eByPolygonVertex) fbxNormalIdx = fbxMesh->GetPolygonVertexIndex(polygonIdx) + positionInPolygon;
	else fbxNormalIdx = fbxMesh->GetPolygonVertex(polygonIdx, positionInPolygon);

	switch (fbxNormal->GetReferenceMode())
	{
	case FbxLayerElement::eDirect:

		normalVector = fbxNormal->GetDirectArray().GetAt(fbxNormalIdx);

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
			// uv같은 경우에는 특이하게 eByPolygonVertex일 때 
			// reference mode를 무시하고 GetTextureUVIndex를 통해 바로 directarray의 index를 얻어와야 한다.

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

SSGeometryAsset* SSFBXImporter::GenerateGeometryFromFbxMesh(::FbxMesh* fbxMesh)
{
	assert(fbxMesh != nullptr);
	SSGeometryAsset* NewGeometryAsset = DBG_NEW SSGeometryAsset();

	// - Load num
	const uint32 layerNum = fbxMesh->GetLayerCount();
	const uint32 ControlPointCnt = fbxMesh->GetControlPointsCount();
	const uint32 PolygonCount = fbxMesh->GetPolygonCount();
	const uint32 PolygonVertexNum = fbxMesh->GetPolygonVertexCount();
	const FbxGeometryElementNormal* const FbxNormal = fbxMesh->GetElementNormal();
	SS_ASSERT(FbxNormal != nullptr, "normal must be exists.");


	// ControlPointToSSIdxMap[ControlPointIdx][배열에 들어온대로의 순서] = SSVertexBuffer의Idx
	// i번째 ControlPoint에 해당되는(물리적 위치가 같은) SSVertexBufferIdx의 리스트를 들고있음
	SS::PooledList<SS::PooledList<uint32>> ControlPointToSSIdxMap(ControlPointCnt);
	ControlPointToSSIdxMap.Resize(ControlPointCnt);
	for (SS::PooledList<uint32>& item : ControlPointToSSIdxMap)
	{
		constexpr uint32 PLENTY_VALUE_FOR_EACH_IDX_MAP = 10;
		item.IncreaseCapacityAndCopy(PLENTY_VALUE_FOR_EACH_IDX_MAP);
	}



	uint32 uvChannelCnt = fbxMesh->GetUVLayerCount();
	FbxGeometryElementUV* fbxUV[VERTEX_UV_MAP_COUNT_MAX];
	if (uvChannelCnt > VERTEX_UV_MAP_COUNT_MAX)
	{
		SS_CLASS_WARNING_LOG("Too many uv channel");
		uvChannelCnt = VERTEX_UV_MAP_COUNT_MAX;
	}
	for (uint32 i = 0; i < uvChannelCnt; i++)
	{
		fbxUV[i] = fbxMesh->GetElementUV(i);
		SS_ASSERT(fbxUV[i] != nullptr, "uv must be exists of idx %d", i);
	}

	NewGeometryAsset->_meshType = EMeshType::Rigid;


	SS::PooledList<SSDefaultVertex> ssVertexBuffer(ControlPointCnt * 2);

	// PolygonVertexToCtrlPointMap[PolygonIdx][배열에 들어온대로의 순서] = <FBX파일의 ControlPoint의 Idx, SSVertexBuffer의 Idx>
	// i번째 Polygon에 해당되는 FBXControlPointIdx와 SSVertexBufferIdx의 리스트를 담고있음
	SS::PooledList<SS::PooledList<SS::pair<uint32, int32>>> PolygonVertexToCtrlPointMap;
	PolygonVertexToCtrlPointMap.IncreaseCapacityAndCopy(PolygonCount);
	PolygonVertexToCtrlPointMap.Resize(PolygonCount);


	// - Build SSVertex, FbxVertex related Map
	for (uint32 i = 0; i < PolygonCount; i++)
	{
		uint32 PolygonVertexCount = fbxMesh->GetPolygonSize(i);
		PolygonVertexToCtrlPointMap[i].IncreaseCapacityAndCopy(PolygonVertexCount);
		for (uint32 j = 0; j < PolygonVertexCount; j++)
		{
			uint32 fbxFileControlPointIdx = -1;
			SSDefaultVertex extractedVertex = ExtractVertex(fbxMesh, i, j, fbxFileControlPointIdx);
			SS_ASSERT(fbxFileControlPointIdx != -1, "Invalid ControlPoint");

			int32 ssVertexBufferIdx = -1;
			for (uint32 k = 0; k < ControlPointToSSIdxMap[fbxFileControlPointIdx].GetSize(); k++)
			{
				uint32 ssIdx = ControlPointToSSIdxMap[fbxFileControlPointIdx][k];
				if (AreSimilarVertex(ssVertexBuffer[ssIdx], extractedVertex))
				{
					ssVertexBufferIdx = k;
					break;
				}
			}

			if (ssVertexBufferIdx == -1)
			{
				// 아래 코드대로 대입되면 PolygonVertexToCtrlPointMap[PolygonIdx][배열에 들어온대로의 순서] = <FBX파일의 ControlPoint의 Idx, SSVertexBuffer의 Idx>가 됨.
				PolygonVertexToCtrlPointMap[i].PushBack({ fbxFileControlPointIdx, (int32)ControlPointToSSIdxMap[fbxFileControlPointIdx].GetSize() });

				// ControlPointToSSIdxMap[ControlPointIdx][배열에 들어온대로의 순서] = SSVertexBuffer의Idx
				ControlPointToSSIdxMap[fbxFileControlPointIdx].PushBackCapacity(ssVertexBuffer.GetSize());

				// SimilarVertex가 없을땐 FbxMesh의 PolygonVeretex를 하나씩 돌면서 SSVertex 버퍼에 값을 차곡차곡 채워넣음
				ssVertexBuffer.PushBackCapacity(extractedVertex);
			}
			else
			{
				// SimilarVertex가 있으면 해당 FBX Vertex의 
				PolygonVertexToCtrlPointMap[i].PushBack({ fbxFileControlPointIdx, ssVertexBufferIdx });
			}
		}
	}


	// - alloc vertex memory
	NewGeometryAsset->_vertexCnt = ssVertexBuffer.GetSize();
	NewGeometryAsset->_eachVertexDataSize = sizeof(SSDefaultVertex);
	uint32 validVertexBufferSize = NewGeometryAsset->_eachVertexDataSize * NewGeometryAsset->_vertexCnt;
	NewGeometryAsset->_vertexData = DBG_MALLOC(validVertexBufferSize);
	SSDefaultVertex* ssVertex = (SSDefaultVertex*)NewGeometryAsset->_vertexData;

	// - copy to real time vertex buffer
	memcpy_s(ssVertex, validVertexBufferSize, ssVertexBuffer.GetData(), validVertexBufferSize);


	// - alloc index memory
	if (fbxMesh->GetNode()->GetMaterial(0) != nullptr)
		NewGeometryAsset->_subGeometryNum = fbxMesh->GetNode()->GetMaterialCount();
	else
		NewGeometryAsset->_subGeometryNum = 1;
	assert(NewGeometryAsset->_subGeometryNum < SUBGEOM_COUNT_MAX);

	FbxGeometryElementMaterial* fbxElementMaterial = fbxMesh->GetElementMaterial();

	FbxLayerElementArrayTemplate<int>* materialIndices = nullptr;

	// https://blog.naver.com/PostView.naver?blogId=lifeisforu&logNo=80105592736
	// FBX SDK 공식 문서에서도 찾아볼 수 있는데 보통 eByPolygon과 eAllSame만이 Material을 위한 index로 사용된다.
	// 즉, 특정 fbxMesh에서 Polygon마다 다른 Material이 할당될 수 있다는 것은 다른 Material을 사용하는 SubGeometry가 필요할 수 있다는 것이다.
	if (fbxMesh->GetElementMaterial() != nullptr && fbxMesh->GetElementMaterial()->GetMappingMode() == FbxLayerElement::eByPolygon)
	{
		fbxMesh->GetMaterialIndices(&materialIndices);

		for (uint32 i = 0; i < PolygonCount; i++) // 메테리얼이 1개 이상이니까 SubGeomtry별로 나눔
		{
			uint8 matIdx = materialIndices->GetAt(i);
			NewGeometryAsset->_indexDataNum[matIdx] += ((fbxMesh->GetPolygonSize(i) - 2) * 3);
		}
	}
	else
	{
		for (uint32 i = 0; i < PolygonCount; i++) // 메테리얼 1개 고정이니까 그냥 다 더함
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

	if (fbxMesh->GetElementMaterial() != nullptr && fbxMesh->GetElementMaterial()->GetMappingMode() == FbxLayerElement::eByPolygon) // 만약 머티리얼이 여러개면
	{
		for (uint32 i = 0; i < PolygonCount; i++)
		{
			uint32 PolygonVertexCount = fbxMesh->GetPolygonSize(i);
			uint32 matIdx = materialIndices->GetAt(i);
			uint32 idxDataStart = NewGeometryAsset->_indexDataStartIndex[matIdx];
			for (uint32 j = 1; j < PolygonVertexCount - 1; j++)
			{
				SS::pair<uint32, int32> CtrlPointIdx = PolygonVertexToCtrlPointMap[i][0];
				uint32 ssVertexBufferIdx = ControlPointToSSIdxMap[CtrlPointIdx.first][CtrlPointIdx.second];
				NewGeometryAsset->_indexData[idxDataStart + subMaterialIdxDataCounter[matIdx]] = ssVertexBufferIdx;

				CtrlPointIdx = PolygonVertexToCtrlPointMap[i][j];
				ssVertexBufferIdx = ControlPointToSSIdxMap[CtrlPointIdx.first][CtrlPointIdx.second];
				NewGeometryAsset->_indexData[idxDataStart + subMaterialIdxDataCounter[matIdx] + 1] = ssVertexBufferIdx;

				CtrlPointIdx = PolygonVertexToCtrlPointMap[i][j + 1];
				ssVertexBufferIdx = ControlPointToSSIdxMap[CtrlPointIdx.first][CtrlPointIdx.second];
				NewGeometryAsset->_indexData[idxDataStart + subMaterialIdxDataCounter[matIdx] + 2] = ssVertexBufferIdx;

				subMaterialIdxDataCounter[matIdx] += 3;
			}
		}
	}
	else // 만약 머티리얼이 하나면
	{
		for (uint32 i = 0; i < fbxMesh->GetPolygonCount(); i++)
		{
			uint32 thisPolygonSize = fbxMesh->GetPolygonSize(i);
			for (uint32 j = 1; j < thisPolygonSize - 1; j++)
			{
				SS::pair<uint32, int32> CtrlPointIdx = PolygonVertexToCtrlPointMap[i][j + 1];
				uint32 ssVertexBufferIdx = ControlPointToSSIdxMap[CtrlPointIdx.first][CtrlPointIdx.second];
				NewGeometryAsset->_indexData[subMaterialIdxDataCounter[0]] = ssVertexBufferIdx;

				CtrlPointIdx = PolygonVertexToCtrlPointMap[i][j];
				ssVertexBufferIdx = ControlPointToSSIdxMap[CtrlPointIdx.first][CtrlPointIdx.second];
				NewGeometryAsset->_indexData[subMaterialIdxDataCounter[0] + 1] = ssVertexBufferIdx;

				CtrlPointIdx = PolygonVertexToCtrlPointMap[i][0];
				ssVertexBufferIdx = ControlPointToSSIdxMap[CtrlPointIdx.first][CtrlPointIdx.second];
				NewGeometryAsset->_indexData[subMaterialIdxDataCounter[0] + 2] = ssVertexBufferIdx;

				subMaterialIdxDataCounter[0] += 3;
			}
		}
	}

	for (uint32 i = 0; i < NewGeometryAsset->_subGeometryNum; i++)
		assert(NewGeometryAsset->_indexDataNum[i] == subMaterialIdxDataCounter[i]);



	// Calculate tangent if there is no tangent data on FbxFormat
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
				SSDefaultVertex& v0 = ssVertex[thisIdxData[i]];
				SSDefaultVertex& v1 = ssVertex[thisIdxData[i + 1]];
				SSDefaultVertex& v2 = ssVertex[thisIdxData[i + 2]];

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

	if (fbxNormal->GetMappingMode() == FbxLayerElement::eByPolygonVertex) fbxNormalIdx = fbxMesh->GetPolygonVertexIndex(polygonIdx) + positionInPolygon;
	else fbxNormalIdx = fbxMesh->GetPolygonVertex(polygonIdx, positionInPolygon);

	switch (fbxNormal->GetReferenceMode())
	{
	case FbxLayerElement::eDirect:

		normalVector = fbxNormal->GetDirectArray().GetAt(fbxNormalIdx);

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
			// uv같은 경우에는 특이하게 eByPolygonVertex일 때 
			// reference mode를 무시하고 GetTextureUVIndex를 통해 바로 directarray의 index를 얻어와야 한다.

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
		constexpr uint32 PLENTY_VALUE_FOR_EACH_IDX_MAP = 10;
		item.IncreaseCapacityAndCopy(PLENTY_VALUE_FOR_EACH_IDX_MAP);
	}

	SS::PooledList<SS::PooledList<SS::pair<uint32, int32>>> PolygonVertexToCtrlPointMap;

	uint32 uvChannelCnt = fbxMesh->GetUVLayerCount();
	FbxGeometryElementUV* fbxUV[VERTEX_UV_MAP_COUNT_MAX];
	if (uvChannelCnt > VERTEX_UV_MAP_COUNT_MAX)
	{
		SS_CLASS_ERR_LOG("Too many uv channel");
		uvChannelCnt = VERTEX_UV_MAP_COUNT_MAX;
	}
	for (uint32 i = 0; i < uvChannelCnt; i++)
	{
		fbxUV[i] = fbxMesh->GetElementUV(i);
		SS_ASSERT(fbxUV[i] != nullptr, "uv must be exists of idx %d", i);
	}


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
			for (uint32 k = 0; k < ControlPointToSSIdxMap[ControlPointIdx].GetSize(); k++)
			{
				uint32 ssIdx = ControlPointToSSIdxMap[ControlPointIdx][k];
				if (AreSimilarVertex(ssVertexBuffer[ssIdx], extractedVertex))
				{
					ctrlPointListIdx = k;
					break;
				}
			}

			if (ctrlPointListIdx == -1)
			{
				// 아래 코드대로 대입되면 PolygonVertexToCtrlPointMap[PolygonIdx][PolygonVertexIdx] = SSVertexBuffer의 Idx가 됨.
				PolygonVertexToCtrlPointMap[i].PushBack({ ControlPointIdx, (int32)ControlPointToSSIdxMap[ControlPointIdx].GetSize() });

				// ControlPointToSSIdxMap[ControlPointIdx] = SSVertexBuffer의Idx
				ControlPointToSSIdxMap[ControlPointIdx].PushBackCapacity(ssVertexBuffer.GetSize());

				// SimilarVertex가 없을땐 FbxMesh의 PolygonVeretex를 하나씩 돌면서 SSVertex 버퍼에 값을 차곡차곡 채워넣음
				ssVertexBuffer.PushBackCapacity(extractedVertex);
			}
			else
			{
				// SimilarVertex가 있으면 해당 FBX Vertex의 
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
