
#include "SSRenderer.h"

#include "SSCamera.h"
#include "../ExternalUtils/ExternalUtils.h"
#include "SSDebug.h"



HRESULT SSRenderer::Init(HINSTANCE InhInst, HWND InhWnd)
{
	HRESULT hr = S_OK;

	hInst = InhInst;
	hWnd = InhWnd;

	GetClientRect(InhWnd, &WindowRect);
	uint32 width = WindowRect.right - WindowRect.left;
	uint32 height = WindowRect.bottom - WindowRect.top;

	uint32 createDeviceFlags = 0;

#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE
	};
	uint32 numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	uint32 numFeatureLevels = ARRAYSIZE(featureLevels);


	for (uint8 driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++) {

		DriverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, DriverType, nullptr, createDeviceFlags
			, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &D3DDevice, &FeatureLevel, &DeviceContext);

		if (hr == E_INVALIDARG) {
			// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
			hr = D3D11CreateDevice(nullptr, DriverType, nullptr
				, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, &D3DDevice, &FeatureLevel, &DeviceContext);
		}

		if (SUCCEEDED(hr)) {
			break;
		}
	}
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	IDXGIFactory1* dxgiFactory = nullptr;
	{
		IDXGIDevice* dxgiDevice = nullptr;
		hr = D3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

		if (SUCCEEDED(hr)) {

			IDXGIAdapter* adapter = nullptr;
			hr = dxgiDevice->GetAdapter(&adapter);
			if (SUCCEEDED(hr)) {
				hr = adapter->GetParent(__uuidof(IDXGIFactory1), (void**)&dxgiFactory);
				adapter->Release();
			}
			dxgiDevice->Release();
		}
	}
	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	IDXGIFactory2* dxgiFactory2 = nullptr;
	hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), (void**)&dxgiFactory2);
	if (dxgiFactory2) {
		// DirectX 11.1 or later
		hr = D3DDevice->QueryInterface(__uuidof(ID3D11Device1), (void**)&D3DDevice1);
		if (SUCCEEDED(hr)) {
			(void)DeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&DeviceContext1);
		}

		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 1; // 멀티샘플링(Anti Aliasing 관련 옵션들)
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1; // 스왑 체인이 소유할 버퍼의 개수를 적어준다.

		hr = dxgiFactory2->CreateSwapChainForHwnd(D3DDevice, hWnd, &sd, nullptr, nullptr, &SwapChain1);
		if (SUCCEEDED(hr))
		{
			hr = SwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&SwapChain));
		}
		dxgiFactory2->Release();
	}
	else {
		//	DirectX 11.0 version
		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferCount = 1; // 보통 프론트버퍼를 포함한 총 버퍼의 개수를 적어준다. 
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 스왑체인 컬러포맷
		sd.BufferDesc.RefreshRate.Numerator = 60; // 60프레임
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hWnd;
		sd.SampleDesc.Count = 1; // SampleDesc: 멀티샘플링 옵션들(MSAA)
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE; // 윈도우 모드인지 풀스크린 모드인지

		hr = dxgiFactory->CreateSwapChain(D3DDevice, &sd, &SwapChain);
	}

	// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
	dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	dxgiFactory->Release();

	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	hr = D3DDevice->CreateRenderTargetView(pBackBuffer, nullptr, &RenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = D3DDevice->CreateTexture2D(&descDepth, nullptr, &DepthStencil);

	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = D3DDevice->CreateDepthStencilView(DepthStencil, &descDSV, &DepthStencilView);
	// 위에서 만든 텍스쳐로 뎁스 스텐실 "뷰"를 만듦
	if (FAILED(hr))
		return hr;

	DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);


	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	DeviceContext->RSSetViewports(1, &vp);

	// 셰이더 초기화 코드완료!
	// 메테리얼 초기화, 버텍스버퍼 초기화 함수 만들기

	hr = InitShaderManager();
	if (FAILED(hr)) {
		SS_LOG("Error(SSRenderer::InitShaderManager): Shader Manager Init failed.\n");
		return hr;
	}
	
	hr = TextureManager.TempLoadTexture(D3DDevice);
	if (FAILED(hr)) {
		SS_LOG("Error(SSRenderer::TempLoadTexture): Texture loader Init failed.\n");
		return hr;
	}
	
	hr = InitMaterialManager();
	if (FAILED(hr)) {
		SS_LOG("Error(SSRenderer::InitMaterialManager): Material Manager Init failed.\n");
		return hr;
	}
	
	hr = InitGeometryManager();
	if (FAILED(hr)) {
		SS_LOG("Error(SSRenderer::InitGeometryManager): Init geometry manager init failed.");
		return hr;
	}
	
	// TODO: 여기부터 하기


	InitCameraTemp();


	SS_LOG("Log (SSRenderer): Renderer Init finished!\n");




	return S_OK;
}

HRESULT SSRenderer::InitShaderManager()
{

	ShaderManager.Init();
	HRESULT hr = ShaderManager.CompileAllShader();
	if (FAILED(hr)) {
		SS_LOG("Error(SSRenderer): Shader compile failed.\n");
		ShaderManager.ReleaseAllShader();
		return hr;
	}

	ShaderManager.InstantiateAllShader(D3DDevice);
	if (FAILED(hr)) {
		SS_LOG("Error(SSRenderer): Shader instantiate failed.\n");
		ShaderManager.ReleaseAllShader();
		return hr;
	}


	return S_OK;
}

HRESULT SSRenderer::InitMaterialManager()
{
	MaterialManager.Init();
	HRESULT hr = MaterialManager.InstantiateAllMaterialsTemp(D3DDevice, &ShaderManager, &TextureManager);
	if (FAILED(hr)) {
		SS_LOG("Error(SSRenderer): Shader compile failed.\n");
		MaterialManager.ReleaseAllMaterialsTemp();
		return hr;
	}

	// HACK: Temp implementation
	{ 
		SSMaterial* mat = MaterialManager.GetMaterialWithIdx(0);
		mat->UpdateWVPMatrix(DeviceContext, XMMatrixIdentity());

		XMVECTOR ZeroVector = XMVectorZero();
		mat->UpdateParameter(1, &ZeroVector, sizeof(XMVECTOR));
	}

	return hr;
}

HRESULT SSRenderer::InitGeometryManager()
{
	GeometryManager.Init();
	HRESULT hr = S_OK;

	GeometryManager.LoadAllGeometryAssetTemp();
	hr = GeometryManager.SendAllGeometryAssetToGPUTemp(D3DDevice);
	if (FAILED(hr)) {
		SS_LOG("Error(SSRenderer): Init Geometry manager failed.\n");
		MaterialManager.ReleaseAllMaterialsTemp();
		return hr;
	}

	GeometryManager.ReleaseAllGeometryDataOnSystem();
	return hr;
}

void SSRenderer::InitCameraTemp()
{
	RenderTarget = new SSCamera();
	RenderTarget->UpdateResolutionWithClientRect(D3DDevice, hWnd);
	RenderTarget->GetTransform().Position = Vector4f(.0f, 3.0f, -12.0f, .0f);
	RenderTarget->SetFOVWithRadians(XM_PIDIV4);
	RenderTarget->SetNearFarZ(0.01f, 100.f);
}

void SSRenderer::CleanUp()
{
	
	GeometryManager.ReleaseAllGeometryDataOnGPU();
	GeometryManager.Release();
	
	MaterialManager.ReleaseAllMaterialsTemp();
	MaterialManager.Release();
	
	TextureManager.ReleaseAllTextures();
	TextureManager.Release();

	ShaderManager.ReleaseAllShader();
	ShaderManager.Release();

	// HACK:임시작업
	{
		delete RenderTarget;
	}

	if (DeviceContext) DeviceContext->ClearState();
	if (D3DDevice) D3DDevice->Release();
	if (D3DDevice1) D3DDevice1->Release();
	if (DeviceContext1) DeviceContext1->Release();
	if (DeviceContext) DeviceContext->Release();
	if (SwapChain) SwapChain->Release();
	if (SwapChain1) SwapChain1->Release();
	if (RenderTargetView) RenderTargetView->Release();
	if (DepthStencil) DepthStencil->Release();
	if (DepthStencilView) DepthStencilView->Release();


}

void SSRenderer::PerFrameTemp()
{
	CalcDeltaTime();
	DeviceContext->ClearRenderTargetView(RenderTargetView, Colors::MidnightBlue);
	DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	SSGeometryAsset* Geometry = GeometryManager.GetGeometryWithIdx(0);
	Geometry->SetDrawTopology(GeometryDrawTopology::TRIANGLELIST);
	Geometry->BindGeometry(DeviceContext);
	{
		Vector4f MeshColor;
		MeshColor.X = (sinf(ElapsedTime * 1.0f) + 1.0f) * 0.5f;
		MeshColor.Y = (cosf(ElapsedTime * 3.0f) + 1.0f) * 0.5f;
		MeshColor.Z = (sinf(ElapsedTime * 5.0f) + 1.0f) * 0.5f;
		MaterialManager.GetMaterialWithIdx(0)->UpdateParameter(1, &MeshColor, sizeof(Vector4f));

		Transform transform;
		transform.Rotation.Y = ElapsedTime;

		XMMATRIX Temp = RenderTarget->GetViewProjMatrix();

		MaterialManager.GetMaterialWithIdx(0)->UpdateWVPMatrix(DeviceContext,
			XMMatrixTranspose(XMMatrixRotationY(ElapsedTime) * RenderTarget->GetViewProjMatrix()));
	}

	SSMaterial* Material = MaterialManager.GetMaterialWithIdx(0);
	Material->BindMaterial(DeviceContext);
	


	uint32 IdxSize = Geometry->GetIndexDataCount();
	DeviceContext->DrawIndexed(IdxSize, 0, 0);



	SwapChain->Present(0,0);
}

void SSRenderer::CalcDeltaTime()
{
	CurTime = GetTickCount64();
	if (StartTime == 0) {
		StartTime = CurTime;
		SS_LOG("Reset Start time\n");
	}
	ElapsedTime = (CurTime - StartTime) / 1000.0f;
}
