
#include "SSRenderer.h"

#include "RenderAsset/AssetType/SSCamera.h"
#include "../ExternalUtils/ExternalUtils.h"
#include "SSEngineDefault/SSDebugLogger.h"
#include "SSEngineDefault/SSInput.h"
#include "SSEngineDefault/SSFrameInfo.h"

#include "SSEngineDefault/SSContainer/StringHashMapA.h"

#include "RenderAsset/SSModelAssetManager.h"
#include "RenderAsset/SSShaderAssetManager.h"
#include "RenderAsset/SSGeometryAssetManager.h"
#include "RenderAsset/SSMaterialAssetManager.h"
#include "RenderAsset/SSTextureAssetManager.h"


#include <vector>

#include "SSSamplerPool.h"
#include "RenderAsset/SSModelCombinationAssetManager.h"


//#define TEMP_FBX_MODEL_PATH "D:\\FBXAssets\\a.fbx"
//#define TEMP_FBX_MODEL_PATH "D:\\FBXAssets\\b.fbx"
//#define TEMP_FBX_MODEL_PATH "D:\\FBXAssets\\c.fbx"
//#define TEMP_FBX_MODEL_PATH "D:\\FBXAssets\\ExportedRoom.fbx"
//#define TEMP_FBX_MODEL_PATH "D:\\FBXAssets\\ExportedRoom02.fbx"
//#define TEMP_FBX_MODEL_PATH "D:\\FBXAssets\\ExportedBox.fbx"
//#define TEMP_FBX_MODEL_PATH "D:\\FBXAssets\\PSController.fbx"
//#define TEMP_FBX_MODEL_PATH "D:\\FBXAssets\\rp_nathan_animated_003_walking.fbx"
#define TEMP_FBX_MODEL_PATH "D:\\FBXAssets\\Frew Worm Monster.fbx"
//#define TEMP_FBX_MODEL_PATH "D:\\FBXAssets\\Ancient Warrior Mixamo Rigged\\source\\Ancient Warrior Mixamo Rigged.fbx"

#define TEMP_MDLC_NAME "Frew Worm Monster.mdlc"
#define TEMP_MDL_NAME "ExportedRoom_PS4 Controller.mdl"


NativePlatformType SSRenderer::GetNativePlatformType()
{
	return NativePlatformType::WindowsD3D11;
}

void* SSRenderer::GetNativeDevice()
{
	return _d3DDevice;
}

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
	uint8 numDriverTypes = ARRAYSIZE(driverTypes);

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
			D3D11_SDK_VERSION, &_d3DDevice, &FeatureLevel, &_deviceContext);

		if (hr == E_INVALIDARG) {
			// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
			hr = D3D11CreateDevice(nullptr, DriverType, nullptr
				, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, &_d3DDevice, &FeatureLevel, &_deviceContext);
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
		hr = _d3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

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
		hr = _d3DDevice->QueryInterface(__uuidof(ID3D11Device1), (void**)&D3DDevice1);
		if (SUCCEEDED(hr)) {
			(void)_deviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&DeviceContext1);
		}

		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 1; // ��Ƽ���ø�(Anti Aliasing ���� �ɼǵ�)
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1; // ���� ü���� ������ ������ ������ �����ش�.

		hr = dxgiFactory2->CreateSwapChainForHwnd(_d3DDevice, hWnd, &sd, nullptr, nullptr, &SwapChain1);
		if (SUCCEEDED(hr))
		{
			hr = SwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&SwapChain));
		}
		dxgiFactory2->Release();
	}
	else {
		//	DirectX 11.0 version
		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferCount = 1; // ���� ����Ʈ���۸� ������ �� ������ ������ �����ش�. 
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // ����ü�� �÷�����
		sd.BufferDesc.RefreshRate.Numerator = 60; // 60������
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hWnd;
		sd.SampleDesc.Count = 1; // SampleDesc: ��Ƽ���ø� �ɼǵ�(MSAA)
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE; // ������ ������� Ǯ��ũ�� �������

		hr = dxgiFactory->CreateSwapChain(_d3DDevice, &sd, &SwapChain);
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

	hr = _d3DDevice->CreateRenderTargetView(pBackBuffer, nullptr, &RenderTargetView);
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
	hr = _d3DDevice->CreateTexture2D(&descDepth, nullptr, &DepthStencil);

	if (FAILED(hr)) {
		__debugbreak();
		return hr;
	}

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = _d3DDevice->CreateDepthStencilView(DepthStencil, &descDSV, &DepthStencilView);
	// ������ ���� �ؽ��ķ� ���� ���ٽ� "��"�� ����
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

	// ���̴� �ʱ�ȭ �ڵ�Ϸ�!
	// ���׸��� �ʱ�ȭ, ���ؽ����� �ʱ�ȭ �Լ� �����

	SSSamplerPool::Instantiate();
	SSSamplerPool::Get()->SetRenderer(this);

	hr = InitShaderManager();
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("Shader Manager init failed.");
		return hr;
	}

	SSTextureAssetManager::Instantiate(100, 1000, 10, RANDOM_PRIMENO_FOR_HASH);
	hr = SSTextureAssetManager::Get()->LoadTextures(_d3DDevice, L"Resource/SerializeAsset/TextureList.json");
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("Texture Loader init failed.");
		return hr;
	}

	SSMaterialAssetManager::Instantiate(100, 1000, 10, RANDOM_PRIMENO_FOR_HASH);
	SSMaterialAssetManager::Get()->CreateTempMaterials(_d3DDevice);


	SSGeometryAssetManager::Instantiate(1000, 1000, 10, RANDOM_PRIMENO_FOR_HASH);
	SSModelAssetManager::Instantiate(1000, 1000, 10, RANDOM_PRIMENO_FOR_HASH);
	SSModelCombinationAssetManager::Instantiate(100, 1000, 10, RANDOM_PRIMENO_FOR_HASH);


	hr = ImportFBXFileToAssetPool();
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG();
		return hr;
	}


	SSMaterialAssetManager::Get()->InstantiateAllMaterials(_d3DDevice, _deviceContext);

	hr = SSGeometryAssetManager::Get()->SendAllGeometryAssetToGPUTemp(_d3DDevice);
	if (FAILED(hr)) {
		SS_LOG("Error(SSRenderer): Init Geometry manager failed.\n");
		SSMaterialAssetManager::Get()->ReleaseAllMaterials();
		return hr;
	}
	SSGeometryAssetManager::Get()->ReleaseAllGeometryDataOnSystem();

	return S_OK;
}

HRESULT SSRenderer::InitShaderManager()
{

	SSShaderAssetManager::Instantiate(100, 1000, 10, RANDOM_PRIMENO_FOR_HASH);
	SSShaderAssetManager::Get()->LoadNewShaderTemp();
	HRESULT hr = SSShaderAssetManager::Get()->CompileAllShader();
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("Shader compile Failed.");
		SSShaderAssetManager::Get()->ReleaseAllShader();
		return hr;
	}

	SSShaderAssetManager::Get()->InstantiateAllShader(_d3DDevice);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("Shader InstantiateGPUBuffer Failed.");
		SS_LOG("Error(SSRenderer): Shader instantiate failed.\n");
		SSShaderAssetManager::Get()->ReleaseAllShader();
		return hr;
	}


	return S_OK;
}




HRESULT SSRenderer::ImportFBXFileToAssetPool()
{
	HRESULT hr = S_OK;

	hr = _fbxImporter.LoadModelAssetFromFBXFile(TEMP_FBX_MODEL_PATH);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG();
		return hr;
	}

	_fbxImporter.StoreCurrentFBXModelAssetToAssetManager();

	return hr;
}


void SSRenderer::InitCameraTemp()
{
	_renderTarget = DBG_NEW SSCamera();
	_renderTarget->UpdateResolutionWithClientRect(_d3DDevice, hWnd);
	_renderTarget->GetTransform().Position = Vector4f(.0f, .0f, -5.0f, .0f);
	_renderTarget->GetTransform().Rotation = Quaternion::FromLookDirect(Vector4f::Zero - _renderTarget->GetTransform().Position);
	_renderTarget->SetFOVWithRadians(XM_PIDIV4);
	_renderTarget->SetNearFarZ(0.01f, 10000.f);

	SS_LOG("Log (SSRenderer): Renderer Init finished!\n");
}


void SSRenderer::CleanUp()
{

	SSModelCombinationAssetManager::Get()->ReleaseAllAssets();
	SSModelCombinationAssetManager::Release();

	SSModelAssetManager::Get()->ReleaseAllModels();
	SSModelAssetManager::Release();

	SSGeometryAssetManager::Get()->ReleaseAllGeometryDataOnGPU();
	SSGeometryAssetManager::Release();

	SSMaterialAssetManager::Get()->ReleaseAllMaterials();
	SSMaterialAssetManager::Get()->Release();

	SSTextureAssetManager::Get()->ReleaseAllTextures();
	SSTextureAssetManager::Release();

	SSShaderAssetManager::Get()->ReleaseAllShader();
	SSShaderAssetManager::Release();

	SSSamplerPool::Release();

	// HACK: 
	{
		delete _renderTarget;
	}

	if (_deviceContext) _deviceContext->ClearState();
	if (_d3DDevice) _d3DDevice->Release();
	if (D3DDevice1) D3DDevice1->Release();
	if (DeviceContext1) DeviceContext1->Release();
	if (_deviceContext) _deviceContext->Release();
	if (SwapChain) SwapChain->Release();
	if (SwapChain1) SwapChain1->Release();
	if (RenderTargetView) RenderTargetView->Release();
	if (DepthStencil) DepthStencil->Release();
	if (DepthStencilView) DepthStencilView->Release();


}

void SSRenderer::BeginFrame()
{
	InitCameraTemp();
}

void SSRenderer::PerFrame()
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
		constexpr float CAM_SPEED = 3;
		constexpr float CAM_ROT_SPEED = 0.5;
		constexpr float CAM_ZOOM_SPEED = 1;

		if (SSInput::GetKeyDown(EKeyCode::KEY_W))
			SS_LOG("W Key down!\n");

		if (SSInput::GetKeyUp(EKeyCode::KEY_W))
			SS_LOG("W Key up!\n");

		if (SSInput::GetKey(EKeyCode::KEY_W))
			_renderTarget->GetTransform().Position = _renderTarget->GetTransform().Position + _renderTarget->GetTransform().GetForward() * SSFrameInfo::GetDeltaTime() * CAM_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_S))
			_renderTarget->GetTransform().Position = _renderTarget->GetTransform().Position + _renderTarget->GetTransform().GetBackward() * SSFrameInfo::GetDeltaTime() * CAM_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_A))
			_renderTarget->GetTransform().Position = _renderTarget->GetTransform().Position + _renderTarget->GetTransform().GetLeft() * SSFrameInfo::GetDeltaTime() * CAM_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_D))
			_renderTarget->GetTransform().Position = _renderTarget->GetTransform().Position + _renderTarget->GetTransform().GetRight() * SSFrameInfo::GetDeltaTime() * CAM_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_E))
			_renderTarget->GetTransform().Position = _renderTarget->GetTransform().Position + _renderTarget->GetTransform().GetUp() * SSFrameInfo::GetDeltaTime() * CAM_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_Q))
			_renderTarget->GetTransform().Position = _renderTarget->GetTransform().Position + _renderTarget->GetTransform().GetDown() * SSFrameInfo::GetDeltaTime() * CAM_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_C))
			_renderTarget->SetFOVWithRadians(_renderTarget->GetRadianFOV() - SSFrameInfo::GetDeltaTime() * CAM_ZOOM_SPEED);

		if (SSInput::GetKey(EKeyCode::KEY_Z))
			_renderTarget->SetFOVWithRadians(_renderTarget->GetRadianFOV() + SSFrameInfo::GetDeltaTime() * CAM_ZOOM_SPEED);

		if (SSInput::GetKey(EKeyCode::KEY_RIGHT))
			_camYRotation += SSFrameInfo::GetDeltaTime() * CAM_ROT_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_LEFT))
			_camYRotation -= SSFrameInfo::GetDeltaTime() * CAM_ROT_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_UP))
		{
			if (_camXRotation > -XM_PIDIV2 * CAM_XROT_MAX)
				_camXRotation -= SSFrameInfo::GetDeltaTime() * CAM_ROT_SPEED;
		}

		if (SSInput::GetKey(EKeyCode::KEY_DOWN))
		{
			if (_camXRotation < XM_PIDIV2 * CAM_XROT_MAX)
				_camXRotation += SSFrameInfo::GetDeltaTime() * CAM_ROT_SPEED;
		}
	}


	_camYRotation = fmodf(_camYRotation, XM_2PI);
	_renderTarget->GetTransform().Rotation = XMQuaternionRotationRollPitchYaw(_camXRotation, _camYRotation, 0);
	_globalParamContext.VPMatrix = XMMatrixTranspose(_renderTarget->GetViewProjMatrix());
	_globalParamContext.SunDirection = Vector4f(1,1,0,0);
	_globalParamContext.SunIntensity = Vector4f::One * 0.05;


	SSModelCombinationAsset* mdlComb = SSModelCombinationAssetManager::Get()->FindAssetWithName(TEMP_MDLC_NAME);
	SSModelAsset* mdl = SSModelAssetManager::FindModelWithName(TEMP_MDL_NAME);

	Transform trans;

	TraverseModelCombinationAndDraw(mdlComb, trans.AsMatrix(), trans.Rotation.AsMatrix());

	SwapChain->Present(0, 0);
}

void SSRenderer::TraverseModelCombinationAndDraw(SSPlaceableAsset* asset, XMMATRIX transformMatrix, XMMATRIX rotMatrix)
{

	// HACK: model combination�� ������ �ƴ϶� ������ ���� ���� ��Ʈ��常 �������� ����
	// HACK: asset->GetParent()!= nullptr ���ֱ�
	if (asset->GetAssetType() == AssetType::ModelCombination && asset->GetParent() != nullptr)
	{
		const SSModelCombinationAsset* modelCombination = static_cast<const SSModelCombinationAsset*>(asset);
		const SSModelAsset* modelAsset = modelCombination->GetModelAsset();

		for (uint8 i = 0; i < modelAsset->GetMultiMaterialCount(); i++)
		{
			modelAsset->BindModel(_deviceContext, i);
			const uint32 idxStart = modelAsset->GetGeometry()->GetIndexDataStartIndex(i);
			const uint32 IdxSize = modelAsset->GetGeometry()->GetIndexDataNum(i);
			SSMaterialAsset* materialAsset = modelAsset->GetMaterial(i);
			materialAsset->UpdateGlobalRenderParam(_deviceContext, _globalParamContext);
			materialAsset->UpdateTransform(_deviceContext, transformMatrix, rotMatrix);

			_deviceContext->DrawIndexed(IdxSize, idxStart, 0);
		}

	}

	for (SSPlaceableAsset* item : asset->GetChilds())
	{
		//		if (strcmp(item->GetAssetName(), "Dirt02") == 0) __debugbreak();
		XMMATRIX mat = item->GetTransform().AsMatrix();
		XMMATRIX matmul = item->GetTransform().AsMatrix() * transformMatrix;
		TraverseModelCombinationAndDraw(item,
			item->GetTransform().AsMatrix() * transformMatrix,
			item->GetTransform().Rotation.AsMatrix() * rotMatrix);
	}
}


