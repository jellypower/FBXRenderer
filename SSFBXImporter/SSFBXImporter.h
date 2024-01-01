#pragma once

#include <Windows.h>
#include <fbxsdk.h>


class SSMaterialAssetManager;
class SSGeometryAssetManager;
class SSModelAssetManager;

class SSFBXImporter {

public:
	SSFBXImporter();
	~SSFBXImporter();

	HRESULT LoadModelAssetFBXFromFile(const char* FileName);
	void StoreCurrentFBXModelAssetToAssetManager();


	void BindAssetPoolToImportAsset(
		SSMaterialAssetManager* InMaterialManager,
		SSGeometryAssetManager* InGeometryManager,
		SSModelAssetManager* InModelManager);

	void ClearAssetPoolToImportAsset();

private:

	SSMaterialAssetManager* BoundMaterialManager;
	SSGeometryAssetManager* BoundGeometryManager;
	SSModelAssetManager* BoundModelManager;


	void TraverseNodes();
	void TraverseNodesRecursion(fbxsdk::FbxNode* node);
	void StoreModelToManager(fbxsdk::FbxMesh* InFBXMesh);


private:


	::FbxManager* FBXManagerInst = nullptr;
	::FbxIOSettings* IOSettings = nullptr;
	::FbxImporter* FBXImporterInst = nullptr;

	::FbxScene* CurrentScene = nullptr;

};