#include "SSSkeletonAnimAsset.h"
#include "SSEngineDefault/SSStaticMath.h"
#include "../SSSkeletonAssetManager.h"

SSSkeletonAnimAsset::SSSkeletonAnimAsset(const char* assetName, const char* skeletonAssetName, const uint32 InframeCnt, const uint32 InFramePerSec)
	: SSAssetBase(AssetType::SkeletonAnim), _frameCnt(InframeCnt), _animStack(0), _framePerSec(InFramePerSec)
{
	_assetName = assetName;
	_skeleton = SSSkeletonAssetManager::FindAssetWithName(skeletonAssetName);
	assert(_skeleton);

	uint32 newCapacity = _skeleton->GetBones().GetSize() * _frameCnt;
	_animStack.IncreaseCapacityAndCopy(newCapacity);
	_animStack.Resize(newCapacity);
}

SSSkeletonAnimAsset::SSSkeletonAnimAsset(const char* assetName, SSSkeletonAsset* InSkeletonAsset, const uint32 InframeCnt, const uint32 InFramePerSec)
	: SSAssetBase(AssetType::SkeletonAnim), _frameCnt(InframeCnt), _animStack(0), _framePerSec(InFramePerSec)
{
	_assetName = assetName;
	_skeleton = InSkeletonAsset;

	uint32 newCapacity = _skeleton->GetBones().GetSize() * _frameCnt;
	_animStack.IncreaseCapacityAndCopy(newCapacity);
	_animStack.Resize(newCapacity);
}

uint32 SSSkeletonAnimAsset::GetBoneCnt() const
{
	return _skeleton->GetBones().GetSize();
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
	const Transform* transformBlendList1, const Transform* transformBlendList2, Transform parentTransform, uint32 idx, float alpha)
{
	
	Transform thisTransform = SS::Lerp(transformBlendList1[idx] * parentTransform, transformBlendList2[idx] * parentTransform, alpha);

	joints[idx].PosMatrix = XMMatrixTranspose(thisTransform.AsMatrix());
	joints[idx].RotMatrix = XMMatrixTranspose(thisTransform.Rotation.AsMatrix());

	for (uint32 childIdx : bones[idx]._childs)
	{
		UpdateJointBufferRecursive(joints, bones, transformBlendList1, transformBlendList2, thisTransform, childIdx, alpha);
	}
}

void SSSkeletonAnimAsset::UpdateGPUBufferFrameState(ID3D11DeviceContext* InDeviceContext, float time)
{
	D3D11_MAPPED_SUBRESOURCE dataPtr;
	HRESULT hr = InDeviceContext->Map(_jointBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr);
	if (SUCCEEDED(hr))
	{
		JOINTMATRIX* const joints = (JOINTMATRIX*)dataPtr.pData;

		uint32 boneCnt = _skeleton->GetBones().GetSize();

		const float frameTime = GetFrameTime();
		time = fmod(time, frameTime);

		const uint32 blendTransformIdx1 = (uint32)(time / GetFrameTimeInterval());
		uint32 blendTransformIdx2 = (blendTransformIdx1 + 1);
		blendTransformIdx2 = blendTransformIdx2 >= _frameCnt ? blendTransformIdx1 : blendTransformIdx2;

		float alpha = fmod(time, GetFrameTimeInterval());
		alpha = alpha / GetFrameTimeInterval();

		UpdateJointBufferRecursive(
			joints,
			_skeleton->GetBones(),
			_animStack.GetData() + (boneCnt * blendTransformIdx1),
			_animStack.GetData() + (boneCnt * blendTransformIdx2),
			Transform::Identity,
			0,
			alpha);		


		InDeviceContext->Unmap(_jointBuffer, 0);
	}
	else
	{
		SS_CLASS_ERR_LOG("Buffer update failed.");
		return;
	}
}

void SSSkeletonAnimAsset::UpdateGPUBufferFrameStateTime(ID3D11DeviceContext* InDeviceContext, float frameTime)
{
	// TODO: 별도의 버퍼를 두고 선형 메모리에 미리 업데이트를 다 해놓은 다음에 실제로 GPU에 카피하는 방식으로 작동하게 하자
	// 지금은 일단 Animation 에셋이랑 인스턴스가 분리돼있지만 나중에는 이를 합치자.
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
