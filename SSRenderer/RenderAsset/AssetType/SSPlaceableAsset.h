#pragma once
#include "SSHierarchyAsset.h"
#include "SSEngineDefault/SSEngineDefault.h"

class SSPlaceableAsset : public SSAssetBase
{
	friend class SSFBXImporter;
protected:
	Transform _transform;
	SSPlaceableAsset* _parent;
	SS::PooledList<SSPlaceableAsset*> _childs;

public:
	SSPlaceableAsset(AssetType assetType, uint32 childCnt = 0);
	virtual ~SSPlaceableAsset() override;

	FORCEINLINE const Transform& GetTransform() const { return _transform; }
	FORCEINLINE const SS::PooledList<SSPlaceableAsset*>& GetChilds() const { return _childs; }
	FORCEINLINE const SSPlaceableAsset* GetParent() const { return _parent; }
};