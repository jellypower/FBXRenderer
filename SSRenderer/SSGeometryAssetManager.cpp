#include "SSGeometryAssetManager.h"
#include "SSEngineDefault/SSDebugLogger.h"

SSGeometryAssetManager* SSGeometryAssetManager::g_instance = nullptr;

SSGeometryAssetManager::SSGeometryAssetManager(uint32 poolSize)
{
	GeometryList = DBG_NEW SSGeometryAsset * [poolSize];
	_geometryPoolCapacity = poolSize;
}

SSGeometryAssetManager::~SSGeometryAssetManager()
{
	for (int i = 0; i < _geometryPoolCount; i++) {
		delete GeometryList[i];
	}

	delete[] GeometryList;
}

void SSGeometryAssetManager::InstantiateNewGeometry(FbxMesh* InFbxMesh)
{
	SSGeometryAsset* NewAsset;
	SS_LOG("%s: %d\n", InFbxMesh->GetNode()->GetName(), _geometryPoolCount);
	GeometryList[_geometryPoolCount++] = NewAsset = DBG_NEW SSGeometryAsset();


	if (_geometryPoolCount > _geometryPoolCapacity) {
		SS_CLASS_WARNING_LOG("Pool is full");
		return;
	}

	NewAsset->InitGeometryDataOnSystem(InFbxMesh);

}

HRESULT SSGeometryAssetManager::SendAllGeometryAssetToGPUTemp(ID3D11Device* InDevice)
{

	HRESULT hr = GeometryList[0]->UpdateDataOnGPU(InDevice);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG();
		return hr;
	}

	hr = GeometryList[0]->SendIndexDataOnGPU(InDevice);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG();
		return hr;
	}

	for (int i = 1; i < _geometryPoolCount; i++) {
		hr = GeometryList[i]->UpdateDataOnGPU(InDevice);
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
	for (int i = 0; i < _geometryPoolCount; i++) {
		GeometryList[i]->ReleaseVertexDataOnSystem();
		GeometryList[i]->ReleaseIndexDataOnSystem();
	}
}

void SSGeometryAssetManager::ReleaseAllGeometryDataOnGPU()
{
	for (int i = 0; i < _geometryPoolCount; i++) {
		GeometryList[i]->ReleaseVertexDataOnGPU();
		GeometryList[i]->ReleaseIndexDataOnGPU();
	}
}
