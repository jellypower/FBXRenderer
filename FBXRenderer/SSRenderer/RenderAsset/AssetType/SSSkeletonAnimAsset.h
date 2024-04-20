#pragma once
#include <d3d11.h>

#include "SSEngineDefault/SSEngineDefault.h"
#include "SSEngineDefault/SSContainer/PooledList.h"
#include "SSAssetBase.h"

class SSSkeletonAsset;

class SSSkeletonAnimAsset : public SSAssetBase
{
	friend class SSFBXImporter;

private:
	SSSkeletonAsset* _skeleton = nullptr;

	SS::PooledList<Transform> _animStack;
	ID3D11Buffer* _jointBuffer = nullptr;
	ID3D11ShaderResourceView* _jointBufferSRV = nullptr;
	uint32 _frameCnt = 0;



public:

	SSSkeletonAnimAsset(const char* assetName, const char* skeletonAssetName, const uint32 InframeCnt);
	SSSkeletonAnimAsset(const char* assetName, SSSkeletonAsset* InSkeletonAsset, const uint32 InframeCnt);

public:
	void InstantiateGPUBuffer(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext);
	void UpdateGPUBufferFrameState(ID3D11DeviceContext* InDeviceContext, uint32 curFrameIdx);
	FORCEINLINE ID3D11Buffer* const* GetJointBufferPtr() const { return &_jointBuffer; }
	FORCEINLINE ID3D11ShaderResourceView* const* GetJointBufferSRVPtr() const { return &_jointBufferSRV; }
private:
	void SetAnimStackTransform(uint32 skeletonIdx, uint32 frameIdx, const Transform& InTransform);


	
};

