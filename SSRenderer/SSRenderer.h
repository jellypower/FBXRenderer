#pragma once

#include <windows.h>
#include <d3d11_1.h>
#include <directxmath.h>
#include <directxcolors.h>

#include "../resource.h"
#include "SSNativeTypes.h"
#include "SSShaderAssetManager.h"
#include "SSMaterialAssetManager.h"
#include "SSTextureManager.h"
#include "SSGeometryAssetManager.h"


class SSRenderer {
public:

	HRESULT Init(HINSTANCE InhInst, HWND InhWnd);
	HRESULT InitShaderManager();
	HRESULT InitMaterialManager();
	HRESULT InitGeometryManager();
	void InitCameraTemp();
	void CleanUp();

	// HACK: PerFrame µû·Î »©³õ±â
	void PerFrameTemp();


	float ElapsedTime = 0;
	float CurTime = 0;
	float StartTime = 0;
	void CalcDeltaTime();
private:

private:
	HINSTANCE hInst = NULL;
	HWND hWnd = NULL;

	ID3D11Device* D3DDevice = nullptr;
	ID3D11Device1* D3DDevice1 = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	ID3D11DeviceContext* DeviceContext1 = nullptr;
	IDXGISwapChain* SwapChain = nullptr;
	IDXGISwapChain1* SwapChain1 = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;
	
	ID3D11Texture2D* DepthStencil = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;



	D3D_DRIVER_TYPE DriverType = D3D_DRIVER_TYPE_UNKNOWN;
	D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_1_0_CORE;

	RECT WindowRect = {0,0,0,0};

	
	SSShaderAssetManager ShaderManager;
	SSMaterialAssetManager MaterialManager;
	SSTextureManager TextureManager;
	SSGeometryAssetManager GeometryManager;

	class SSCamera* RenderTarget;


};