#pragma once
#include <d3d11.h>

#include "SSAssetBase.h"
#include "SSGeometryAsset.h"

class SSGeometryAsset;
class SSMaterialAsset;


class SSModelAsset : public SSAssetBase
{
	friend class SSFBXImporter;
private:

	SSGeometryAsset* _geometry = nullptr;
	SSMaterialAsset* _material = nullptr;

public:
	SSModelAsset(const char* InAssetName, SSGeometryAsset* InGeometry, SSMaterialAsset* InMaterial);
	SSModelAsset(const char* InAssetName, const char* InGeometryName, const char* InMaterialName);
	virtual ~SSModelAsset() override;

	FORCEINLINE SSGeometryAsset* GetGeometry() const { return _geometry;  }
	FORCEINLINE SSMaterialAsset* GetMaterial() const { return _material; }

	void BindModel(ID3D11DeviceContext* InDeviceContext) const;



};
