#pragma once
#include "SSPlaceableAsset.h"
#include "SSModelAsset.h"

class SSModelCombinationAsset : public SSPlaceableAsset
{
	friend class SSFBXImporter;
private:
	SSModelAsset* _modelAsset;

public:
	SSModelCombinationAsset(const char* InAssetName, SSModelAsset* modelAsset, uint32 childCnt);
	SSModelCombinationAsset(const char* InAssetName, const char* modelName, uint32 childCnt);
	virtual ~SSModelCombinationAsset() override;

	FORCEINLINE const SSModelAsset* GetModelAsset() const { return _modelAsset; }

};

