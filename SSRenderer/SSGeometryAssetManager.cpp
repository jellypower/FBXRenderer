#include "SSGeometryAssetManager.h"
#include "SSDebugLogger.h"

void SSGeometryAssetManager::Init()
{
	GeometryList = new SSGeometryAsset * [DEFAULT_POOL_SIZE];
}

void SSGeometryAssetManager::Release()
{

	for (int i = 0; i < GeometryPoolCount; i++) {
		delete GeometryList[i];
	}

	delete[] GeometryList;
}

void SSGeometryAssetManager::LoadAllGeometryAssetTemp()
{
	GeometryList[GeometryPoolCount++] = new SSGeometryAsset();
	
	GeometryList[0]->InitVertexDataOnSystem();
	GeometryList[0]->LoadIndexDataOnSystem();

}

HRESULT SSGeometryAssetManager::SendAllGeometryAssetToGPUTemp(ID3D11Device* InDevice)
{
	
	HRESULT hr = GeometryList[0]->SendVertexDataOnGPU(InDevice);
	if (FAILED(hr)) {
		SS_LOG("Error(SSGeometryAssetManager::LoadAllGeometryAssetTemp): \
			Vertex Creation failed on GPU.\n");
		return hr;
	}

	hr = GeometryList[0]->SendIndexDataOnGPU(InDevice);
	if (FAILED(hr)) {
		SS_LOG("Error(SSGeometryAssetManager::LoadAllGeometryAssetTemp): \
			Index data Creation failed on GPU.\n");
		return hr;
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
