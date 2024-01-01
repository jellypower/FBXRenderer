
#include "SSFBXImporter.h"
#include "SSEngineDefault/SSDebugLogger.h"
#include "SSRenderer/SSMaterialAssetManager.h"
#include "SSRenderer/SSGeometryAssetManager.h"




SSFBXImporter::SSFBXImporter()
{
	FBXManagerInst = ::FbxManager::Create();

	IOSettings = FbxIOSettings::Create(FBXManagerInst, IOSROOT);

	IOSettings->SetBoolProp(IMP_FBX_MATERIAL, true); // import 관련 설정
	FBXManagerInst->SetIOSettings(IOSettings);

	FBXImporterInst = fbxsdk::FbxImporter::Create(FBXManagerInst, "");

}

SSFBXImporter::~SSFBXImporter()
{
	FBXImporterInst->Destroy();
	IOSettings->Destroy();
	FBXManagerInst->Destroy();
}

HRESULT SSFBXImporter::LoadModelAssetFBXFromFile(const char* FileName)
{

	if (!FBXImporterInst->Initialize(FileName, -1, FBXManagerInst->GetIOSettings())) {
		SS_CLASS_WARNING_LOG("%s", FBXImporterInst->GetStatus().GetErrorString());
		return E_FAIL;
	}

	CurrentScene = FbxScene::Create(FBXManagerInst, FileName);
	FBXImporterInst->Import(CurrentScene);

	return S_OK;

}

void SSFBXImporter::StoreCurrentFBXModelAssetToAssetManager()
{
	if (BoundMaterialManager == nullptr || BoundGeometryManager == nullptr || BoundModelManager == nullptr) {
		SS_CLASS_WARNING_LOG("Manager not bound");
		return;
	}

	TraverseNodes();
}

void SSFBXImporter::BindAssetPoolToImportAsset(SSMaterialAssetManager* InMaterialManager, SSGeometryAssetManager* InGeometryManager, SSModelAssetManager* InModelManager)
{
	BoundMaterialManager = InMaterialManager;
	BoundGeometryManager = InGeometryManager;
	BoundModelManager = InModelManager;
}

void SSFBXImporter::ClearAssetPoolToImportAsset()
{
	BoundMaterialManager = nullptr;
	BoundGeometryManager = nullptr;
	BoundModelManager = nullptr;
}
void SSFBXImporter::TraverseNodes()
{
	if (CurrentScene == nullptr) {
		SS_CLASS_WARNING_LOG("No scene to load");
		return;
	}

	FbxNode* rootNode = CurrentScene->GetRootNode();

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
	if (BoundGeometryManager == nullptr) {
		SS_CLASS_WARNING_LOG("No Bound Geometry Manager");
		return;
	}

	BoundGeometryManager->InstantiateNewGeometry(InFBXMesh);
	
}

