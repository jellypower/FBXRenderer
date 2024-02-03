
#include "SSModelAsset.h"

#include "SSMaterialAsset.h"
#include "SSGeometryAsset.h"
#include "../SSGeometryAssetManager.h"
#include "../SSMaterialAssetManager.h"

#include "SSEngineDefault/SSDebugLogger.h"


SSModelAsset::SSModelAsset(const char* InAssetName, SSGeometryAsset* InGeometry)
	: SSAssetBase(AssetType::Model)
{
	_geometry = InGeometry;
	_assetName = InAssetName;
}

SSModelAsset::SSModelAsset(const char* InAssetName, const char* InGeometryName)
	: SSAssetBase(AssetType::Model)
{
	_geometry = SSGeometryAssetManager::FindGeometryWithName(InGeometryName);
	_assetName = InAssetName;
}

SSModelAsset::~SSModelAsset()
{
	__noop;
}

void SSModelAsset::SetMaterial(SSMaterialAsset* InMaterial, uint32 matIdx)
{
	if(matIdx > _geometry->GetSubGeometryNum())
	{
		SS_CLASS_ERR_LOG("material idx out of bounds");
		return;
	}

	_multiMaterialList[matIdx] = InMaterial;
}

void SSModelAsset::SetMaterial(const char* InMaterialName, uint32 matIdx)
{
	if (matIdx > _geometry->GetSubGeometryNum())
	{
		SS_CLASS_ERR_LOG("material idx out of bounds");
		return;
	}

	SSMaterialAsset* mat = SSMaterialAssetManager::FindAssetWithName(InMaterialName);
	if (mat == nullptr) mat = SSMaterialAssetManager::GetEmptyAsset();

	_multiMaterialList[matIdx] = mat;
}


void SSModelAsset::BindModel(ID3D11DeviceContext* InDeviceContext, uint32 multiMatIdx) const
{
	if (_multiMaterialList[multiMatIdx]->IsBindingPossible()) _multiMaterialList[multiMatIdx]->BindMaterial(InDeviceContext);
	else {
		SS_CLASS_ERR_LOG("material of Asset is not bindable");
		return;
	}

	if (_geometry->UsableOnGPU()) _geometry->BindGeometry(InDeviceContext, multiMatIdx);
	else {
		SS_CLASS_ERR_LOG("material of Asset is not bindable");
		return;
	}
}

