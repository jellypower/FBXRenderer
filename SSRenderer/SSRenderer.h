#pragma once

#include <windows.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include <directxcolors.h>

#include "../resource.h"
#include "SSEngineDefault/SSNativeTypes.h"
#include "SSShaderAssetManager.h"

#include "SSModelAssetManager.h"
#include "SSAsset/SSFBXImporter.h"


class SSRenderer {

private:
	HINSTANCE hInst = NULL;
	HWND hWnd = NULL;

	ID3D11Device* D3DDevice = nullptr;
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

	SSModelAssetManager ModelManager;

	SSFBXImporter _fbxImporter;

	class SSCamera* RenderTarget;

	float _camYRotation = 0;
	float _camXRotation = 0;


public:

	HRESULT Init(HINSTANCE InhInst, HWND InhWnd);
	HRESULT InitShaderManager();
	HRESULT InitMaterialManager();
	HRESULT ImportFBXFileToAssetPool();
	void CleanUp();


	void BeginFrame();

	void PerFrame();

private:


	void InitCameraTemp();


};