
#include "SSModelAsset.h"

#include "SSMaterialAsset.h"
#include "SSGeometryAsset.h"

#include "SSEngineDefault/SSDebugLogger.h"


void SSModelAsset::InitTemp(SSMaterialAsset* InMaterial, SSGeometryAsset* InGeometry)
{
	if (InMaterial == nullptr) {
		SS_CLASS_ERR_LOG("No Material");
		return;
	}
	if (InGeometry == nullptr) {
		SS_CLASS_ERR_LOG("No Geometry");
		return;
	}

	Material = InMaterial;
	Geometry = InGeometry;

	InstanceStage = ModelAssetInstanceStage::MaterialGeometryBound;
}

void SSModelAsset::BindModel(ID3D11DeviceContext* InDeviceContext)
{
	if (Material->IsBindingPossible()) Material->BindMaterial(InDeviceContext);
	else {
		SS_CLASS_ERR_LOG("Material of Asset is not bindable");
		return;
	}

	if (Geometry->IsBindingPossible()) Geometry->BindGeometry(InDeviceContext);
	else {
		SS_CLASS_ERR_LOG("Material of Asset is not bindable");
		return;
	}
}

void SSModelAsset::Release()
{
	Material = nullptr;
	Geometry = nullptr;
	InstanceStage = ModelAssetInstanceStage::JustCreated;
}

