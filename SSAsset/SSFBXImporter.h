#pragma once

#include <SSEngineDefault/SSEngineDefault.h>
#include <fbxsdk.h>

#include "SSEngineDefault/SSContainer/SSUtilityContainer.h"


class SSGeometryAsset;
class SSMaterialAssetManager;
class SSModelAssetManager;
class SSModelAsset;

class SSFBXImporter {
private:
	::FbxManager* _FBXManager = nullptr;
	::FbxIOSettings* _IOSetting = nullptr;
	::FbxImporter* _FBXImporter = nullptr;
	::FbxScene* _currentScene = nullptr;
	SS::FixedStringA<PATH_LEN_MAX> _filePath;

public:
	SSFBXImporter();
	~SSFBXImporter();

	HRESULT LoadModelAssetFromFBXFile(const char* filePath);
	void ClearFBXModelAsset();

	void StoreCurrentFBXModelAssetToAssetManager();


private:
	void TraverseNodes();
	void TraverseNodesRecursion(fbxsdk::FbxNode* node);
	void StoreModelToManager(fbxsdk::FbxMesh* InFBXMesh);

	SSGeometryAsset* GenerateGeometryFromFbxMesh(fbxsdk::FbxMesh* fbxMesh);
};