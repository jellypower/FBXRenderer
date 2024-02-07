#pragma once
#include "SSEngineDefault/SSEngineDefault.h"
#include "SSEngineDefault/SSContainer/PooledList.h"

#include "SSAssetBase.h"

class SSHierarchyAsset : public SSAssetBase
{
protected:
	SSHierarchyAsset* _parent;
	SS::PooledList<SSHierarchyAsset*> _childs;


protected:
	SSHierarchyAsset(AssetType assetType, uint32 childCapacity=0);

	virtual ~SSHierarchyAsset() = 0 { }
};

