
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

HRESULT SSFBXImporter::LoadModelAssetFBXFromFile(const char* FileName)
{

	if (!_FBXImporter->Initialize(FileName, -1, _FBXManager->GetIOSettings())) {
		SS_CLASS_WARNING_LOG("%s", _FBXImporter->GetStatus().GetErrorString());
		return E_FAIL;
	}

	_currentScene = FbxScene::Create(_FBXManager, FileName);
	_FBXImporter->Import(_currentScene);

	return S_OK;

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

void SSFBXImporter::TraverseNodesRecursion(FbxNode* node)
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

