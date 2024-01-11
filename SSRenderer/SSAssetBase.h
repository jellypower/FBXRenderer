#pragma once
#include "SSNonCopyable.h"
#include "SSEngineDefault/SSContainer/SSUtilityContainer.h"

enum class AssetType
{
	None,

	Geometry = 1,


	Count
};

class SSAssetBase : public SSNoncopyable
{
protected:
	SS::FixedStringA<ASSET_NAME_LEN_MAX> _assetName;
	SS::FixedStringA<PATH_LEN_MAX> _assetPath;
	AssetType _assetType = AssetType::None;

protected:
	virtual ~SSAssetBase() = 0 { };
};
