#pragma once

#include <windows.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include <directxcolors.h>

#include "../resource.h"
#include "SSEngineDefault/SSNativeTypes.h"

#include "RenderAsset/SSFBXImporter.h"
#include "RenderAsset/AssetType/SSMaterialAsset.h"


struct BoneNode;

enum class NativePlatformType
{
	WindowsD3D11,
};


class SSRenderer {

private:
	HINSTANCE hInst = NULL;
	HWND hWnd = NULL;

	ID3D11Device* _d3DDevice = nullptr;
	ID3D11Device1* D3DDevice1 = nullptr;
	ID3D11DeviceContext* _deviceContext = nullptr;
	ID3D11DeviceContext* DeviceContext1 = nullptr;
	IDXGISwapChain* SwapChain = nullptr;
	IDXGISwapChain1* SwapChain1 = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;

	ID3D11Texture2D* DepthStencil = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;

	D3D_DRIVER_TYPE DriverType = D3D_DRIVER_TYPE_UNKNOWN;
	D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_1_0_CORE;

	RECT WindowRect = { 0,0,0,0 };

	SSFBXImporter _fbxImporter;

	SS::FixedStringW<PATH_LEN_MAX> _fbxFilePath;
	SS::FixedStringA<ASSET_NAME_LEN_MAX> _mdlcAssetName;

	class SSCamera* _renderTarget;

	float _camYRotation = 0;
	float _camXRotation = 0;

	

	// HACK: 
	struct PbrMaterialParamEditorCopy* _pbrMaterialParamcopyList = nullptr;

	class SSModelCombinationAsset* _mdlcCache;
	class SSSkeletonAsset* _skeletonAssetCache;
	class SSSkeletonAnimAsset* _skeletonAnimCache;
	class SSModelAsset* _directionMeshCache;

	GlobalRenderParam _globalParamContext;

public:
	virtual NativePlatformType GetNativePlatformType();
	void* GetNativeDevice();

	HRESULT Init(HINSTANCE InhInst, HWND InhWnd);
	HRESULT InitShaderManager();

	void BindFbxFilePathToImport(const utf16* FilePath);
	HRESULT ImportFBXFileToAssetPool();
	void CleanUp();

	void BindModel(const SSModelAsset* modelToBind, uint8 multiMaterialIdx = 0);

	void SetModelTransform(Transform boundAssetTransform);
	void Draw();


	void BeginFrame();
	void PerFrame();

private:
	void BindMaterial(SSMaterialAsset* materialAsset);
	void BindGeometry(SSGeometryAsset* geometryAsset);
	void BindShader(SSShaderAsset* shaderAsset);

	void BindSkeleton(SSSkeletonAsset* skeletonAsset);
	void BindSkeletonAnim(class SSSkeletonAnimAsset* skeletonAnimAsset, float time);

	void TraverseModelCombinationAndDraw(SSPlaceableAsset* asset, XMMATRIX transformMatrix, XMMATRIX rotMatrix);
	void TraverseSkeletonAndDrawRecursion(const SS::PooledList<BoneNode>& boneList, const uint16 boneIdx, XMMATRIX transformMatrix, XMMATRIX rotMatrix);
	void DrawAnimatedSkeleton(const SS::PooledList<BoneNode>& boneList, const float frameTime, const uint16 boneIdx, Transform transform);
	// Skeleton 자체를 그리는 함수임 SkinnedMesh그리는 함수 아님
	void DrawAnimatedSkeletonRecursion(const SS::PooledList<BoneNode>& boneList, const Transform* animTransform, const uint16 boneIdx, Transform transform);
	
};
