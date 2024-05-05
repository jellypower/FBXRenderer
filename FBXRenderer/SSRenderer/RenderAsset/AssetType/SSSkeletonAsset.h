#pragma once
#include <d3d11.h>

#include "SSPlaceableAsset.h"


struct JOINTMATRIX
{
	XMMATRIX PosMatrix;
	XMMATRIX RotMatrix;
};

struct BoneNode
{
	SS::FixedStringA<ASSET_NAME_LEN_MAX> _boneName;
	Transform _transform;
	float _length;
	uint16 _parent;
	SS::PooledList<uint16> _childs;

	BoneNode(const char* boneName, uint16 parentIdx, uint16 childCnt);
	BoneNode(BoneNode&& boneNode);
	BoneNode(const BoneNode& boneNode);
};

class SSSkeletonAsset : public SSAssetBase
{
	friend class SSFBXImporter;

protected:
	SS::PooledList<BoneNode> _boneList;
	ID3D11Buffer* _jointBuffer = nullptr;
	ID3D11ShaderResourceView* _jointBufferSRV = nullptr;

public:
	SSSkeletonAsset(const char* InAssetName, uint16 boneCount);

	FORCEINLINE const SS::PooledList<BoneNode>& GetBones() const { return _boneList; }
	FORCEINLINE bool IsBindingPossible() const { _jointBuffer != nullptr; }


	FORCEINLINE ID3D11Buffer* const* GetJointBufferPtr() const { return &_jointBuffer; }
	FORCEINLINE ID3D11ShaderResourceView* const* GetJointBufferSRVPtr() const { return &_jointBufferSRV; }

	void InstantiateGPUBuffer(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext);
	void ReleaseGPUBuffer();
};

