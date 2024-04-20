#include "SSSkeletonAsset.h"


BoneNode::BoneNode(const char* boneName, uint16 parentIdx, uint16 childCnt) :
	_boneName(boneName), _parent(parentIdx), _childs(childCnt)
{
}

BoneNode::BoneNode(BoneNode&& boneNode) :
	_boneName(boneNode._boneName),
	_parent(boneNode._parent),
	_childs(SS::move(boneNode._childs))
{

}


SSSkeletonAsset::SSSkeletonAsset(const char* InAssetName, uint16 boneCount) :
	SSAssetBase(AssetType::Skeleton), _boneList(boneCount)
{
	_assetName = InAssetName;
}

void SSSkeletonAsset::InstantiateGPUBuffer(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext)
{
	HRESULT hr;
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = sizeof(XMMATRIX) * _boneList.GetSize();
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(XMMATRIX);
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hr = InDevice->CreateBuffer(&desc, nullptr, &_jointBuffer);
	if (FAILED(hr))
	{
		SS_CLASS_ERR_LOG("Buffer creation failed.");
		return;
	}

	D3D11_MAPPED_SUBRESOURCE dataPtr;
	hr = InDeviceContext->Map(_jointBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr);
	if(SUCCEEDED(hr))
	{
		XMMATRIX* joints = (XMMATRIX*)dataPtr.pData;

		for (uint32 i = 0; i < _boneList.GetSize(); i++)
		{
			joints[i]= _boneList[i]._transform.AsInverseMatrix();
		}

		InDeviceContext->Unmap(_jointBuffer, 0);
	}
	else
	{
		SS_CLASS_ERR_LOG("Buffer update failed.");
		return;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = _boneList.GetSize();

	hr = InDevice->CreateShaderResourceView( _jointBuffer , &srvDesc, &_jointBufferSRV);
	if (FAILED(hr))
	{
		SS_CLASS_ERR_LOG("SRV creation failed.");
		return;
	}


}

void SSSkeletonAsset::ReleaseGPUBuffer()
{
	for (int i = 0; i < _boneList.GetSize(); i++) {
		if (_jointBuffer != nullptr)
			_jointBuffer->Release();
		_jointBuffer = nullptr;
	}
}
