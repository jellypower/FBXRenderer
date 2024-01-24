#pragma once
#include "SSEngineDefault/SSNonCopyable.h"
#include "SSEngineDefault/SSContainer/SSUtilityContainer.h"

enum class AssetType
{
	None = 0,

	Blank,
	Geometry,
	Shader,
	Material,
	Model,
	ModelCombination,


	Count
};

class SSAssetBase : public SSNoncopyable
{
protected:
	SSAssetBase(AssetType assetType);

	SS::FixedStringA<ASSET_NAME_LEN_MAX> _assetName;
	SS::FixedStringW<PATH_LEN_MAX> _assetPath;
	const AssetType _assetType;

public:
	FORCEINLINE const char* GetAssetName() const { return _assetName.GetData(); }
	FORCEINLINE const utf16* GetAssetPath() const { return _assetPath.GetData(); }
	FORCEINLINE const AssetType GetAssetType() const { return _assetType; }

protected:
	virtual ~SSAssetBase() = 0 { };
};

