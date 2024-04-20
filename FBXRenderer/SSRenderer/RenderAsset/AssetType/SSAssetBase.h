﻿#pragma once
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
	Texture,
	Skeleton,
	SkeletonAnim,

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
	FORCEINLINE const char* GetAssetName() const { return _assetName.C_Str(); }
	FORCEINLINE const utf16* GetAssetPath() const { return _assetPath.C_Str(); }
	FORCEINLINE const uint32 GetAssetPathLen() const { return _assetPath.GetLen(); }
	FORCEINLINE const AssetType GetAssetType() const { return _assetType; }

protected:
	virtual ~SSAssetBase() = 0 { }
};