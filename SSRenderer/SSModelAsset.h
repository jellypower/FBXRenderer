#pragma once
#include "SSEngineDefault/SSNativeTypes.h"
#include "SSPlaceableObject.h"

#include <d3d11.h>

class SSGeometryAsset;
class SSMaterialAsset;

enum class ModelAssetInstanceStage {
	JustCreated,
	MaterialGeometryBound

};

class SSModelAsset
{
private:
	ModelAssetInstanceStage InstanceStage = ModelAssetInstanceStage::JustCreated;

	SSGeometryAsset* Geometry = nullptr;
	SSMaterialAsset* Material = nullptr;

public:

	void InitTemp(SSMaterialAsset* InMaterial, SSGeometryAsset* InGeometry);
	
	void BindModel(ID3D11DeviceContext* InDeviceContext);

	void Release();

	SSGeometryAsset* GetGeometryAsset() { return Geometry; }


};
