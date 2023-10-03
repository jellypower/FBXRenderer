#pragma once

#include <fbxsdk.h>
#include <Windows.h>

class SSShaderAssetManager;
class SSGeometryAssetManager;

class SSFBXImporter {

public:
	SSFBXImporter();
	~SSFBXImporter();

	HRESULT LoadModelAssetFBXFromFile(const char* FileName);
	void StoreCurrentModelAssetToAssetManager(SSShaderAssetManager* InShaderAssetManager, SSGeometryAssetManager* InGeometryAssetManager);

private:
	::FbxManager* FBXManagerInst = nullptr;
	::FbxIOSettings* IOSettings = nullptr;
	::FbxImporter* FBXImporterInst = nullptr;

	::FbxScene* CurrentScene = nullptr;

};