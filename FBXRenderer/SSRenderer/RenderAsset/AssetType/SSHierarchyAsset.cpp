#include "SSHierarchyAsset.h"

SSHierarchyAsset::SSHierarchyAsset(AssetType assetType, uint32 childCapacity)
	: _parent(nullptr), _childs(childCapacity), SSAssetBase(AssetType::None)
{
}
