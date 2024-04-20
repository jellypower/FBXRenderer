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
	SSMaterialAsset* _multiMaterialList[SUBGEOM_COUNT_MAX] = { 0, };

public:
	SSModelAsset(const char* InAssetName, SSGeometryAsset* InGeometry);
	SSModelAsset(const char* InAssetName, const char* InGeometryName);
	virtual ~SSModelAsset() override;

	FORCEINLINE SSGeometryAsset* GetGeometry() const { return _geometry;  }

	void SetMaterial(SSMaterialAsset* InMaterial, uint32 matIdx);
	void SetMaterial(const char* InMaterialName, uint32 matIdx);
	FORCEINLINE SSMaterialAsset* GetMaterial(uint32 subMatIdx=0) const { return _multiMaterialList[subMatIdx]; }
	FORCEINLINE uint8 GetMultiMaterialCount() const { return _geometry->GetSubGeometryNum(); }

	// 여기서 Skeleton Mesh 타입과 Skeleton Mesh랑 엮이는 SkeletonAsset, SkeletonAnimAsset 바인딩 가능하게 해주자.


};
