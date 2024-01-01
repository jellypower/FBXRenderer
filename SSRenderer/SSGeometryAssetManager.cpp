#include "SSGeometryAssetManager.h"
#include "SSEngineDefault/SSDebugLogger.h"

void SSGeometryAssetManager::Init(int PoolSize)
{
	if (PoolSize == 0) {
		GeometryList = DBG_NEW SSGeometryAsset * [DEFAULT_POOL_SIZE];
		GeometryPoolMax = DEFAULT_POOL_SIZE;
	}
	else {
		GeometryList = DBG_NEW SSGeometryAsset * [PoolSize];
		GeometryPoolMax = PoolSize;
	}
}

void SSGeometryAssetManager::Release()
{

	for (int i = 0; i < GeometryPoolCount; i++) {
		delete GeometryList[i];
	}

	delete[] GeometryList;
}

void SSGeometryAssetManager::InstantiateNewGeometry(FbxMesh* InFbxMesh)
{
	SSGeometryAsset* NewAsset;
	SS_LOG("%s: %d\n", InFbxMesh->GetNode()->GetName(), GeometryPoolCount);
	GeometryList[GeometryPoolCount++] = NewAsset = DBG_NEW SSGeometryAsset();


	if (GeometryPoolCount > GeometryPoolMax) {
		SS_CLASS_WARNING_LOG("Pool is full");
		return;
	}

	NewAsset->InitGeometryDataOnSystem(InFbxMesh);

}

HRESULT SSGeometryAssetManager::SendAllGeometryAssetToGPUTemp(ID3D11Device* InDevice)
{

	HRESULT hr = GeometryList[0]->SendVertexDataOnGPU(InDevice);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG();
		return hr;
	}

	hr = GeometryList[0]->SendIndexDataOnGPU(InDevice);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG();
		return hr;
	}

	for (int i = 1; i < GeometryPoolCount; i++) {
		hr = GeometryList[i]->SendVertexDataOnGPU(InDevice);
		if (FAILED(hr)) {
			SS_CLASS_ERR_LOG();
			return hr;
		}

		hr = GeometryList[i]->SendIndexDataOnGPU(InDevice);
		if (FAILED(hr)) {
			SS_CLASS_ERR_LOG();
			return hr;
		}
	}

	return S_OK;
}

void SSGeometryAssetManager::ReleaseAllGeometryDataOnSystem()
{
	for (int i = 0; i < GeometryPoolCount; i++) {
		GeometryList[i]->ReleaseVertexDataOnSystem();
		GeometryList[i]->ReleaseIndexDataOnSystem();
	}
}

void SSGeometryAssetManager::ReleaseAllGeometryDataOnGPU()
{
	for (int i = 0; i < GeometryPoolCount; i++) {
		GeometryList[i]->ReleaseVertexDataOnGPU();
		GeometryList[i]->ReleaseIndexDataOnGPU();
	}
}
