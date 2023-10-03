
#include "SSFBXImporter.h"
#include "SSDebugLogger.h"
#include "SSRenderer/SSShaderAssetManager.h"
#include "SSRenderer/SSGeometryAssetManager.h"


SSFBXImporter::SSFBXImporter()
{
	FBXManagerInst = ::FbxManager::Create();

	IOSettings = FbxIOSettings::Create(FBXManagerInst, IOSROOT);

	IOSettings->SetBoolProp(IMP_FBX_MATERIAL, true); // import 관련 설정
	FBXManagerInst->SetIOSettings(IOSettings);

	FBXImporterInst = ::FbxImporter::Create(FBXManagerInst, "");

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
		SS_CLASS_WARNING_LOG("Call to FbxImporter::Initialize() failed.\n");
		SS_CLASS_WARNING_LOG("\t%s", FBXImporterInst->GetStatus().GetErrorString());
		return E_FAIL;
	}

	CurrentScene = FbxScene::Create(FBXManagerInst, FileName);
	FBXImporterInst->Import(CurrentScene);

}

void SSFBXImporter::StoreCurrentModelAssetToAssetManager(SSShaderAssetManager* InShaderAssetManager, SSGeometryAssetManager* InGeometryAssetManager)
{
	
}

