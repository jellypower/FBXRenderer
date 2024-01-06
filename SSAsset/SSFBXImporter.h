#pragma once

#include <Windows.h>
#include <fbxsdk.h>


class SSMaterialAssetManager;
class SSModelAssetManager;

class SSFBXImporter {

public:
	SSFBXImporter();
	~SSFBXImporter();

	HRESULT LoadModelAssetFBXFromFile(const char* FileName);
	void StoreCurrentFBXModelAssetToAssetManager();


private:
	void TraverseNodes();
	void TraverseNodesRecursion(fbxsdk::FbxNode* node);
	void StoreModelToManager(fbxsdk::FbxMesh* InFBXMesh);


private:
	::FbxManager* _FBXManager = nullptr;
	::FbxIOSettings* _IOSetting = nullptr;
	::FbxImporter* _FBXImporter = nullptr;

	::FbxScene* _currentScene = nullptr;

};