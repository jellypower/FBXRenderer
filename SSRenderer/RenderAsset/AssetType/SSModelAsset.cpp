
#include "SSModelAsset.h"

#include "SSMaterialAsset.h"
#include "SSGeometryAsset.h"
#include "../SSGeometryAssetManager.h"
#include "../SSMaterialAssetManager.h"

#include "SSEngineDefault/SSDebugLogger.h"


SSModelAsset::SSModelAsset(const char* InAssetName, SSGeometryAsset* InGeometry,SSMaterialAsset* InMaterial)
	: SSAssetBase(AssetType::Model)
{
	_geometry = InGeometry;
	_material = InMaterial;

	_assetName = InAssetName;
}

SSModelAsset::SSModelAsset(const char* InAssetName, const char* InGeometryName, const char* InMaterialName)
	: SSAssetBase(AssetType::Model)
{
	_geometry = SSGeometryAssetManager::FindGeometryWithName(InGeometryName);
	_material = SSMaterialAssetManager::FindAssetWithName(InMaterialName);

	_assetName = InAssetName;
}

SSModelAsset::~SSModelAsset()
{
	__noop;
}


void SSModelAsset::BindModel(ID3D11DeviceContext* InDeviceContext) const
{
	if (_material->IsBindingPossible()) _material->BindMaterial(InDeviceContext);
	else {
		SS_CLASS_ERR_LOG("material of Asset is not bindable");
		return;
	}

	if (_geometry->UsableOnGPU()) _geometry->BindGeometry(InDeviceContext);
	else {
		SS_CLASS_ERR_LOG("material of Asset is not bindable");
		return;
	}
}

