/*#pragma once
#include "SSEngineDefault/SSNativeTypes.h"

#include <d3d11.h>

class SSGeometryAsset;
class SSMaterial;

enum class ModelAssetInstanceStage {
	JustCreated,
	MaterialGeometryBound

};

class SSModelAsset
{
public:
	SSModelAsset(SSMaterial* InMaterial, SSGeometryAsset* InGeometry);
	
	void BindModel(ID3D11DeviceContext* InDeviceContext);


private:

	ModelAssetInstanceStage InstanceStage = ModelAssetInstanceStage::JustCreated;

	SSGeometryAsset* Geometry = nullptr;
	SSMaterial* Material = nullptr;

	Transform transform;

};

*/