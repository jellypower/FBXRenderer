#include "SSSkeletonAnimAsset.h"
#include "../SSSkeletonAssetManager.h"

SSSkeletonAnimAsset::SSSkeletonAnimAsset(const char* assetName, const char* skeletonAssetName, const uint32 InframeCnt)
	: SSAssetBase(AssetType::SkeletonAnim), _frameCnt(InframeCnt), _animStack(0)
{
	_assetName = assetName;
	_skeleton = SSSkeletonAssetManager::FindAssetWithName(skeletonAssetName);
	assert(_skeleton);

	uint32 newCapacity = _skeleton->GetBones().GetSize() * _frameCnt;
	_animStack.IncreaseCapacityAndCopy(newCapacity);
	_animStack.Resize(newCapacity);
}

SSSkeletonAnimAsset::SSSkeletonAnimAsset(const char* assetName, SSSkeletonAsset* InSkeletonAsset, const uint32 InframeCnt)
	: SSAssetBase(AssetType::SkeletonAnim), _frameCnt(InframeCnt), _animStack(0)
{
	_assetName = assetName;
	_skeleton = InSkeletonAsset;

	uint32 newCapacity = _skeleton->GetBones().GetSize() * _frameCnt;
	_animStack.IncreaseCapacityAndCopy(newCapacity);
	_animStack.Resize(newCapacity);
}

void SSSkeletonAnimAsset::InstantiateGPUBuffer(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext)
{
	HRESULT hr;
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = sizeof(JOINTMATRIX) * _skeleton->GetBones().GetSize();
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(JOINTMATRIX);
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hr = InDevice->CreateBuffer(&desc, nullptr, &_jointBuffer);
	if (FAILED(hr))
	{
		SS_CLASS_ERR_LOG("Buffer creation failed.");
		return;
	}


	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = _skeleton->GetBones().GetSize();

	hr = InDevice->CreateShaderResourceView(_jointBuffer, &srvDesc, &_jointBufferSRV);
	if (FAILED(hr))
	{
		SS_CLASS_ERR_LOG("SRV creation failed.");
		return;
	}
}

void SSSkeletonAnimAsset::ReleaseGPUBuffer()
{
	if (_jointBuffer != nullptr)
	{
		_jointBuffer->Release();
		_jointBufferSRV->Release();
	}
}

static void UpdateJointBufferRecursive(JOINTMATRIX* joints, const SS::PooledList<BoneNode>& bones,
	const Transform* transformList, Transform parentTransform, uint32 idx)
{
	Transform thisTransform = transformList[idx] * parentTransform;

	joints[idx].PosMatrix = XMMatrixTranspose(thisTransform.AsMatrix());
	joints[idx].RotMatrix = XMMatrixTranspose(thisTransform.Rotation.AsMatrix());

	for (uint32 childIdx : bones[idx]._childs)
	{
		UpdateJointBufferRecursive(joints, bones, transformList, thisTransform, childIdx);
	}
}

void SSSkeletonAnimAsset::UpdateGPUBufferFrameState(ID3D11DeviceContext* InDeviceContext, uint32 curFrameIdx)
{
	D3D11_MAPPED_SUBRESOURCE dataPtr;
	HRESULT hr = InDeviceContext->Map(_jointBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr);
	if (SUCCEEDED(hr))
	{
		JOINTMATRIX* const joints = (JOINTMATRIX*)dataPtr.pData;

		uint32 boneCnt = _skeleton->GetBones().GetSize();

		UpdateJointBufferRecursive(joints, _skeleton->GetBones(), _animStack.GetData() + (boneCnt * curFrameIdx), Transform::Identity, 0);		


		InDeviceContext->Unmap(_jointBuffer, 0);
	}
	else
	{
		SS_CLASS_ERR_LOG("Buffer update failed.");
		return;
	}
}


static void ResetJointBufferRecursive(JOINTMATRIX* joints, const SS::PooledList<BoneNode>& bones, Transform parentTransform, uint32 idx)
{
	Transform thisTransform =  bones[idx]._transform * parentTransform;

	joints[idx].PosMatrix = XMMatrixTranspose(thisTransform.AsMatrix());
	joints[idx].RotMatrix = XMMatrixTranspose(thisTransform.Rotation.AsMatrix());
	
	for (uint32 childIdx : bones[idx]._childs)
	{
		ResetJointBufferRecursive(joints, bones, thisTransform, childIdx);
	}
}


void SSSkeletonAnimAsset::ResetJointBufferState(ID3D11DeviceContext* InDeviceContext)
{
	D3D11_MAPPED_SUBRESOURCE dataPtr;
	HRESULT hr = InDeviceContext->Map(_jointBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr);

	if (SUCCEEDED(hr))
	{
		JOINTMATRIX* joints = (JOINTMATRIX*)dataPtr.pData;

		ResetJointBufferRecursive(joints, _skeleton->GetBones(), Transform::Identity, 0);

		InDeviceContext->Unmap(_jointBuffer, 0);
	}
	else
	{
		SS_CLASS_ERR_LOG("Buffer update failed.");
		return;
	}

}

void SSSkeletonAnimAsset::SetAnimStackTransform(uint32 skeletonIdx, uint32 frameIdx, const Transform& InTransform)
{
	_animStack[_skeleton->GetBones().GetSize() * frameIdx + skeletonIdx] = InTransform;
}
