#pragma once
#include "SSEngineDefault/SSNativeTypes.h"

#include <d3d11.h>

class SSGeometryAsset;
class SSMaterialAsset;

enum class ModelAssetInstanceStage {
	JustCreated,
	MaterialGeometryBound

};

class SSModelAsset
{
public:

	void InitTemp(SSMaterialAsset* InMaterial, SSGeometryAsset* InGeometry);
	
	void BindModel(ID3D11DeviceContext* InDeviceContext);


private:

	ModelAssetInstanceStage InstanceStage = ModelAssetInstanceStage::JustCreated;

	SSGeometryAsset* Geometry = nullptr;
	SSMaterialAsset* Material = nullptr;

	Transform transform;

};
