#pragma once

#include "SSEngineDefault/SSEngineDefault.h"

#define FBXSDK_SHARED
#include <fbxsdk.h>

#include "AssetType/SSAssetBase.h"
#include "SSEngineDefault/SSContainer/SSUtilityContainer.h"


class SSGeometryAsset;
class SSMaterialAssetManager;
class SSModelAssetManager;
class SSModelAsset;
class SSPlaceableAsset;

class SSFBXImporter {
private:
	FbxManager* _FBXManager = nullptr;
	FbxIOSettings* _IOSetting = nullptr;
	FbxImporter* _FBXImporter = nullptr;
	FbxScene* _currentScene = nullptr;
	SS::FixedStringA<ASSET_NAME_LEN_MAX> _fileName;
	SS::FixedStringA<PATH_LEN_MAX> _filePath;

public:
	SSFBXImporter();
	~SSFBXImporter();

	HRESULT LoadModelAssetFromFBXFile(const char* filePath);
	void ClearFBXModelAsset();

	void StoreCurrentFBXModelAssetToAssetManager();


private:

	void ImportCurrentSceneToMaterialAsset();

	void ImportCurrentSceneToModelAsset();
	void TraverseNodesRecursion(::FbxNode* node, SSPlaceableAsset* parentAsset);

	SSGeometryAsset* GenerateGeometryFromFbxMesh(::FbxMesh* fbxMesh);

};