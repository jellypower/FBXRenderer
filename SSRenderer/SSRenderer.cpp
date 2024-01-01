
#include "SSRenderer.h"

#include "SSCamera.h"
#include "../ExternalUtils/ExternalUtils.h"
#include "SSEngineDefault/SSDebugLogger.h"
#include "SSEngineDefault/SSInput.h"
#include "SSEngineDefault/SSFrameInfo.h"




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
			D3D11_SDK_VERSION, &D3DDevice, &FeatureLevel, &_deviceContext);

		if (hr == E_INVALIDARG) {
			// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
			hr = D3D11CreateDevice(nullptr, DriverType, nullptr
				, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, &D3DDevice, &FeatureLevel, &_deviceContext);
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
			(void)_deviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&DeviceContext1);
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

	_deviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);


	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	_deviceContext->RSSetViewports(1, &vp);

	// 셰이더 초기화 코드완료!
	// 메테리얼 초기화, 버텍스버퍼 초기화 함수 만들기

	hr = InitShaderManager();
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("Shader Manager init failed.");
		return hr;
	}
	
	hr = TextureManager.TempLoadTexture(D3DDevice);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("Texture Loader init failed.");
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

	hr = ImportFBXFileToAssetPool();
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG();
		return hr;
	}

	hr = GeometryManager.SendAllGeometryAssetToGPUTemp(D3DDevice);
	if (FAILED(hr)) {
		SS_LOG("Error(SSRenderer): Init Geometry manager failed.\n");
		MaterialManager.ReleaseAllMaterialsTemp();
		return hr;
	}

	ModelManager.Init(&MaterialManager, &GeometryManager);
	ModelManager.CreateNewAssetTemp(
		MaterialManager.GetMaterialWithIdx(0), GeometryManager.GetGeometryWithIdx(0));
	

	InitCameraTemp();


	SS_LOG("Log (SSRenderer): Renderer Init finished!\n");




	return S_OK;
}

HRESULT SSRenderer::InitShaderManager()
{

	ShaderManager.Init();
	HRESULT hr = ShaderManager.CompileAllShader();
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("Shader compile Failed.");
		ShaderManager.ReleaseAllShader();
		return hr;
	}

	ShaderManager.InstantiateAllShader(D3DDevice);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("Shader Instantiate Failed.");
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
		SSMaterialAsset* mat = MaterialManager.GetMaterialWithIdx(0);
		mat->UpdateTransform(_deviceContext, XMMatrixIdentity());

//		XMVECTOR ZeroVector = XMVectorZero();
//		mat->UpdateParameter(_deviceContext, 1, &ZeroVector, sizeof(XMVECTOR));
	}

	return hr;
}

HRESULT SSRenderer::InitGeometryManager()
{
	GeometryManager.Init(100);
	HRESULT hr = S_OK;

	return hr;
}

HRESULT SSRenderer::ImportFBXFileToAssetPool()
{
	HRESULT hr = S_OK;
	FbxImporter.BindAssetPoolToImportAsset(&MaterialManager, &GeometryManager, &ModelManager);

	hr = FbxImporter.LoadModelAssetFBXFromFile(
		
//		"D:\\DirectXWorkspace\\OpenFBX\\runtime\\a.fbx"
//		"D:\\DirectXWorkspace\\OpenFBX\\runtime\\b.fbx"
//		"D:\\FBXSDK\\2020.3.4\\samples\\Normals\\Normals.fbx"
		"D:\\DirectXWorkspace\\OpenFBX\\runtime\\Room.fbx"
//		"D:\\DirectXWorkspace\\OpenFBX\\runtime\\Frew Worm Monster.fbx"
	);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG();
		return hr;
	}

	FbxImporter.StoreCurrentFBXModelAssetToAssetManager();

	return hr;
}


void SSRenderer::InitCameraTemp()
{
	RenderTarget = DBG_NEW SSCamera();
	RenderTarget->UpdateResolutionWithClientRect(D3DDevice, hWnd);
	RenderTarget->GetTransform().Position = Vector4f(.0f, 0.0f, -1000.0f, .0f);
	RenderTarget->GetTransform().Rotation = Quaternion::FromLookDirect(Vector4f::Zero - RenderTarget->GetTransform().Position);
	RenderTarget->SetFOVWithRadians(XM_PIDIV4);
	RenderTarget->SetNearFarZ(0.01f, 10000.f);
}


void SSRenderer::CleanUp()
{
	FbxImporter.ClearAssetPoolToImportAsset();
	
	GeometryManager.ReleaseAllGeometryDataOnSystem();
	GeometryManager.ReleaseAllGeometryDataOnGPU();
	GeometryManager.Release();
	
	MaterialManager.ReleaseAllMaterialsTemp();
	MaterialManager.Release();
	
	TextureManager.ReleaseAllTextures();
	TextureManager.Release();

	ShaderManager.ReleaseAllShader();
	ShaderManager.Release();

	ModelManager.ReleaseAllModels();
	ModelManager.Release();



	// HACK:임시작업
	{
		delete RenderTarget;
	}

	if (_deviceContext) _deviceContext->ClearState();
	if (D3DDevice) D3DDevice->Release();
	if (D3DDevice1) D3DDevice1->Release();
	if (DeviceContext1) DeviceContext1->Release();
	if (_deviceContext) _deviceContext->Release();
	if (SwapChain) SwapChain->Release();
	if (SwapChain1) SwapChain1->Release();
	if (RenderTargetView) RenderTargetView->Release();
	if (DepthStencil) DepthStencil->Release();
	if (DepthStencilView) DepthStencilView->Release();


}

void SSRenderer::PerFrameTemp()
{

	_deviceContext->ClearRenderTargetView(RenderTargetView, Colors::MidnightBlue);
	_deviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	constexpr float CAM_XROT_MAX = 0.9;

	// process mouse input
	if (SSInput::GetMouse(EMouseCode::MOUSE_RIGHT))
	{
		constexpr float CAM_ROT_SPEED = 1000;
		
		_camYRotation += SSFrameInfo::GetDeltaTime() * SSInput::GetMouseDelta().X * CAM_ROT_SPEED;

		if (_camXRotation > -XM_PIDIV2 * CAM_XROT_MAX && SSInput::GetMouseDelta().Y > 0)
			_camXRotation -= SSFrameInfo::GetDeltaTime() * SSInput::GetMouseDelta().Y * CAM_ROT_SPEED;

		if (_camXRotation < XM_PIDIV2 * CAM_XROT_MAX && SSInput::GetMouseDelta().Y < 0)
			_camXRotation -= SSFrameInfo::GetDeltaTime() * SSInput::GetMouseDelta().Y * CAM_ROT_SPEED;
		
	}

	// process keyborad input
	{
		constexpr float CAM_SPEED = 100;
		constexpr float CAM_ROT_SPEED = 0.5;

		if (SSInput::GetKey(EKeyCode::KEY_W))
			RenderTarget->GetTransform().Position = RenderTarget->GetTransform().Position + RenderTarget->GetTransform().GetForward() * SSFrameInfo::GetDeltaTime() * CAM_SPEED;
		
		if (SSInput::GetKey(EKeyCode::KEY_S))
			RenderTarget->GetTransform().Position = RenderTarget->GetTransform().Position + RenderTarget->GetTransform().GetBackward() * SSFrameInfo::GetDeltaTime() * CAM_SPEED;
		
		if (SSInput::GetKey(EKeyCode::KEY_A))
			RenderTarget->GetTransform().Position = RenderTarget->GetTransform().Position + RenderTarget->GetTransform().GetLeft() * SSFrameInfo::GetDeltaTime() * CAM_SPEED;
		
		if (SSInput::GetKey(EKeyCode::KEY_D))
			RenderTarget->GetTransform().Position = RenderTarget->GetTransform().Position + RenderTarget->GetTransform().GetRight() * SSFrameInfo::GetDeltaTime() * CAM_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_E))
			RenderTarget->GetTransform().Position = RenderTarget->GetTransform().Position + RenderTarget->GetTransform().GetUp() * SSFrameInfo::GetDeltaTime() * CAM_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_Q))
			RenderTarget->GetTransform().Position = RenderTarget->GetTransform().Position + RenderTarget->GetTransform().GetDown() * SSFrameInfo::GetDeltaTime() * CAM_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_RIGHT))
			_camYRotation += SSFrameInfo::GetDeltaTime() * CAM_ROT_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_LEFT))
			_camYRotation -= SSFrameInfo::GetDeltaTime() * CAM_ROT_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_UP) )
		{
			if(_camXRotation > -XM_PIDIV2 * CAM_XROT_MAX)
				_camXRotation -= SSFrameInfo::GetDeltaTime() * CAM_ROT_SPEED;
		}
			
		if (SSInput::GetKey(EKeyCode::KEY_DOWN))
		{
			if(_camXRotation < XM_PIDIV2 * CAM_XROT_MAX)
				_camXRotation += SSFrameInfo::GetDeltaTime() * CAM_ROT_SPEED;
		}
	}
	_camYRotation = fmodf(_camYRotation, XM_2PI);
	RenderTarget->GetTransform().Rotation = XMQuaternionRotationRollPitchYaw(_camXRotation, _camYRotation, 0);


	SSGeometryAsset* Geometry = GeometryManager.GetGeometryWithIdx(36);
	Geometry->SetDrawTopology(GeometryDrawTopology::TRIANGLELIST);

	SSMaterialAsset* Material = MaterialManager.GetMaterialWithIdx(0);

	Geometry->BindGeometry(_deviceContext);
	Material->BindMaterial(_deviceContext);

	{
		Vector4f MeshColor;
		MeshColor.X = (sinf(SSFrameInfo::GetElapsedTime() * 1.0f) + 1.0f) * 0.5f;
		MeshColor.Y = (cosf(SSFrameInfo::GetElapsedTime() * 3.0f) + 1.0f) * 0.5f;
		MeshColor.Z = (sinf(SSFrameInfo::GetElapsedTime() * 5.0f) + 1.0f) * 0.5f;
		MeshColor = Vector4f(1,1,1,1);

//		Material->UpdateParameter(_deviceContext, 2, &MeshColor, sizeof(Vector4f));

		XMMATRIX Temp = RenderTarget->GetViewProjMatrix();

//		Material->UpdateTransform(_deviceContext, XMMatrixTranspose(XMMatrixRotationY(SSFrameInfo::GetElapsedTime())));

		Material->UpdateCameraSetting(_deviceContext, XMMatrixTranspose(RenderTarget->GetViewProjMatrix()));
	}


	uint32 IdxSize = Geometry->GetIndexDataNum();
	_deviceContext->DrawIndexed(IdxSize, 0, 0);


	SwapChain->Present(0,0);
}


