
#include "SSRenderer.h"

#include "SSCamera.h"
#include "../ExternalUtils/ExternalUtils.h"
#include "SSEngineDefault/SSDebugLogger.h"
#include "SSEngineDefault/SSInput.h"
#include "SSEngineDefault/SSFrameInfo.h"

#include "SSEngineDefault/SSContainer/StringHashMapA.h"
#include "SSEngineDefault/SSContainer/SSUtilityContainer.h"
#include "SSEngineDefault/SSContainer/PooledLinkedList.h"
#include "SSEngineDefault/SSContainer/FixedList.h"

#include "SSGeometryAssetManager.h"
#include "SSMaterialAssetManager.h"
#include "SSTextureManager.h"

#include <vector>

#include "SSEngineDefault/SSContainer/PooledList.h"


//#define TEMP_FBX_MODEL_PATH "D:\\DirectXWorkspace\\OpenFBX\\runtime\\a.fbx"
//#define TEMP_FBX_MODEL_PATH "D:\\DirectXWorkspace\\OpenFBX\\runtime\\b.fbx"
//#define TEMP_FBX_MODEL_PATH "D:\\FBXSDK\\2020.3.4\\samples\\Normals\\Normals.fbx"
#define TEMP_FBX_MODEL_PATH "D:\\DirectXWorkspace\\OpenFBX\\runtime\\Room.fbx"
//#define TEMP_FBX_MODEL_PATH "D:\\DirectXWorkspace\\OpenFBX\\runtime\\rp_nathan_animated_003_walking.fbx"
//#define TEMP_FBX_MODEL_PATH "D:\\DirectXWorkspace\\OpenFBX\\runtime\\Frew Worm Monster.fbx"


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

	SSTextureManager::Instantiate(100);
	hr = SSTextureManager::Get()->TempLoadTexture(D3DDevice);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("Texture Loader init failed.");
		return hr;
	}

	hr = InitMaterialManager();
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("Material Manager Init failed.");
		return hr;
	}

	SSGeometryAssetManager::Instantiate(100);

	hr = ImportFBXFileToAssetPool();
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG();
		return hr;
	}

	hr = SSGeometryAssetManager::Get()->SendAllGeometryAssetToGPUTemp(D3DDevice);
	if (FAILED(hr)) {
		SS_LOG("Error(SSRenderer): Init Geometry manager failed.\n");
		SSMaterialAssetManager::Get()->ReleaseAllMaterialsTemp();
		return hr;
	}

	return S_OK;
}

HRESULT SSRenderer::InitShaderManager()
{

	SSShaderAssetManager::Instantiate(100);
	SSShaderAssetManager::Get()->LoadNewShaderTemp();
	HRESULT hr = SSShaderAssetManager::Get()->CompileAllShader();
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("Shader compile Failed.");
		SSShaderAssetManager::Get()->ReleaseAllShader();
		return hr;
	}

	SSShaderAssetManager::Get()->InstantiateAllShader(D3DDevice);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG("Shader Instantiate Failed.");
		SS_LOG("Error(SSRenderer): Shader instantiate failed.\n");
		SSShaderAssetManager::Get()->ReleaseAllShader();
		return hr;
	}


	return S_OK;
}

HRESULT SSRenderer::InitMaterialManager()
{
	SSMaterialAssetManager::Instantiate(100);
	HRESULT hr = SSMaterialAssetManager::Get()->InstantiateAllMaterialsTemp(D3DDevice);
	if (FAILED(hr)) {
		SS_LOG("Error(SSRenderer): Shader compile failed.\n");
		SSMaterialAssetManager::Get()->ReleaseAllMaterialsTemp();
		return hr;
	}

	// HACK: Temp implementation
	{
		SSMaterialAsset* mat = SSMaterialAssetManager::Get()->GetMaterialWithIdx(0);
		mat->UpdateTransform(_deviceContext, XMMatrixIdentity());

		//		XMVECTOR ZeroVector = XMVectorZero();
		//		mat->UpdateParameter(_deviceContext, 1, &ZeroVector, sizeof(XMVECTOR));
	}

	return hr;
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
	RenderTarget = DBG_NEW SSCamera();
	RenderTarget->UpdateResolutionWithClientRect(D3DDevice, hWnd);
	RenderTarget->GetTransform().Position = Vector4f(.0f, 0.0f, -1000.0f, .0f);
	RenderTarget->GetTransform().Rotation = Quaternion::FromLookDirect(Vector4f::Zero - RenderTarget->GetTransform().Position);
	RenderTarget->SetFOVWithRadians(XM_PIDIV4);
	RenderTarget->SetNearFarZ(0.01f, 10000.f);

	SS_LOG("Log (SSRenderer): Renderer Init finished!\n");
}


void SSRenderer::CleanUp()
{

	SSGeometryAssetManager::Get()->ReleaseAllGeometryDataOnSystem();
	SSGeometryAssetManager::Get()->ReleaseAllGeometryDataOnGPU();
	SSGeometryAssetManager::Release();

	SSMaterialAssetManager::Get()->ReleaseAllMaterialsTemp();
	SSMaterialAssetManager::Get()->Release();

	SSTextureManager::Get()->ReleaseAllTextures();
	SSTextureManager::Release();

	SSShaderAssetManager::Get()->ReleaseAllShader();
	SSShaderAssetManager::Release();

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

void SSRenderer::BeginFrame()
{
	InitCameraTemp();
}

void SSRenderer::PerFrame()
{
	// container example
	{
		srand(time(NULL));
		SS::StringHashMapA<SS::FixedStringA<100>> hashMap(1024, 10, rand());
		for (int i = 0; i < 300; i++) {
			char buffer[100];
			sprintf(buffer, "StringIdx%d", i);
			InsertResult result = hashMap.TryInsert(buffer, buffer);
		}
		bool rebuildResult = hashMap.TryRebuild(2048, 10, rand());

		EraseResult result8 = hashMap.TryErase("StringIdx1");
		SS::FixedStringA<100> outData;
		FindResult result1 = hashMap.TryFind("StringIdx123", outData);
		assert(result1 == FindResult::Success );
		FindResult result2 = hashMap.TryFind("StringIdx12", outData);
		assert(result2 == FindResult::Success);
		FindResult result3 = hashMap.TryFind("StringIdx1", outData);
		FindResult result4 = hashMap.TryFind("StringIdx0", outData);
		assert(result4 == FindResult::Success);
		FindResult result5 = hashMap.TryFind("StringIdx78", outData);
		assert(result5 == FindResult::Success);
		FindResult result6 = hashMap.TryFind("StringIdx68", outData);
		assert(result6 == FindResult::Success);
		FindResult result7 = hashMap.TryFind("StringIdx256", outData);
		assert(result7 == FindResult::Success);


		for (uint32 i = 0; i < 300; i++) {
			char buffer[100];
			sprintf(buffer, "ADSASASDASD%d", i);
			InsertResult result9 = hashMap.TryInsert(buffer, buffer);
			assert(result9 == InsertResult::Success);
		}

		bool result = hashMap.TryRebuild(2048, 10, rand());

		for (uint32 i = 0; i < 300; i++) {
			char buffer[100];
			sprintf(buffer, "ADSASASDASD%d", i);
			SS::FixedStringA<100> outStr;
			FindResult result9 = hashMap.TryFind(buffer, outStr);
			assert(result9 == FindResult::Success);
		}


		SS::StringHashMapA<std::vector<uint32>> vectorHashMap(1024);
		vectorHashMap.TryInsert("ASDF1", std::vector<uint32>(1));
		vectorHashMap.TryInsert("ASDF2", std::vector<uint32>(2));
		vectorHashMap.TryInsert("ASDF3", std::vector<uint32>(3));
		vectorHashMap.TryInsert("ASDF4", std::vector<uint32>(4));



		SS::PooledLinkedList<SS::FixedStringA<100>> linkedList(100);
		linkedList.PushBack("AA10");
		linkedList.PushBack("20");
		linkedList.PushBack("30");
		linkedList.PushBack("AA40");
		linkedList.PushBack("50");
		linkedList.PushBack("60");
		linkedList.PushBack("70");
		linkedList.PushFront("1");
		linkedList.PushFront("2");
		linkedList.PushFront("3");
		linkedList.PushFront("AASAD4");
		linkedList.PushFront("5");
		linkedList.PushFront("6");
		linkedList.PushFront("7");
		linkedList.InsertFront(linkedList.FindIteratorAt(3), "the best before me...");
		linkedList.InsertBack(linkedList.FindIteratorAt(3), "goodbye beombo...");
		linkedList.Erase(linkedList.FindIteratorAt(5));

		for (const SS::FixedStringA<100>&item : linkedList) {
			SS_LOG("linkedlist item: %s \n", item);
		}

		SS::PooledLinkedList<uint32> intList(100);
		intList.PushBack(1);
		intList.PushBack(2);
		intList.PushBack(3);
		intList.PushBack(4);
		intList.PushBack(5);
		intList.PopBack();
		intList.PopFront();






		SS::PooledLinkedList<std::vector<uint32>> vectorLinkedList(8);
		vectorLinkedList.PushBack(std::vector<uint32>(3));
		vectorLinkedList.PushBack(std::vector<uint32>(4));
		vectorLinkedList.PushBack(std::vector<uint32>(5));
		vectorLinkedList.PushBack(std::vector<uint32>(6));
		vectorLinkedList.PushBack(std::vector<uint32>(7));

		std::vector<uint32> myVector(20);
		vectorLinkedList.PushBack(myVector);

		
		SS::FixedList<std::vector<uint32>, 4> vectorList;
		vectorList.PushBack(std::vector<uint32>(10));
		vectorList.PushBack(std::vector<uint32>(20));
		vectorList.PushBack(std::vector<uint32>(30));
		vectorList.PushBack(std::vector<uint32>(40));
		
		for (const std::vector<uint32>& item : vectorList) {
			printf("\t\tvector list size: %d\n", item.size());
		}

		vectorList.Resize(2);
		vectorList.PushBack(std::vector<uint32>(200));
		for (uint32 i = 0; i < vectorList.GetSize(); i++)
			vectorList[i].resize(200);
		vectorList.Clear();

		
		SS::PooledList<std::vector<uint32>> pooledVectorList(4);
		pooledVectorList.PushBack(std::vector<uint32>(10));
		pooledVectorList.PushBack(std::vector<uint32>(30));
		pooledVectorList.PushBack(std::vector<uint32>(20));
		pooledVectorList.PushBack(std::vector<uint32>(40));

		for(const std::vector<uint32>& item : pooledVectorList)
		{
			printf("\t\tpooled vector list size: %d\n", item.size());
		}

		pooledVectorList.IncreaseCapacityAndCopy(10);
		pooledVectorList.Resize(2);
		pooledVectorList.PushBack(std::vector<uint32>(100));

		for (uint32 i = 0; i < pooledVectorList.GetSize(); i++)
			printf("\t\tpooled vector list size: %d\n", pooledVectorList[i].size());
			
		pooledVectorList.Clear();


	}

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
		constexpr float CAM_ZOOM_SPEED = 1;

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

		if (SSInput::GetKey(EKeyCode::KEY_C))
			RenderTarget->SetFOVWithRadians(RenderTarget->GetRadianFOV() - SSFrameInfo::GetDeltaTime() * CAM_ZOOM_SPEED);

		if (SSInput::GetKey(EKeyCode::KEY_Z))
			RenderTarget->SetFOVWithRadians(RenderTarget->GetRadianFOV() + SSFrameInfo::GetDeltaTime() * CAM_ZOOM_SPEED);

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
	RenderTarget->GetTransform().Rotation = XMQuaternionRotationRollPitchYaw(_camXRotation, _camYRotation, 0);


	SSGeometryAsset* Geometry = SSGeometryAssetManager::GetGeometryWithIdx(50);
	Geometry->SetDrawTopology(GeometryDrawTopology::TRIANGLELIST);

	SSMaterialAsset* Material = SSMaterialAssetManager::Get()->GetMaterialWithIdx(0);

	Geometry->BindGeometry(_deviceContext);
	Material->BindMaterial(_deviceContext);

	{
		Vector4f MeshColor;
		MeshColor.X = (sinf(SSFrameInfo::GetElapsedTime() * 1.0f) + 1.0f) * 0.5f;
		MeshColor.Y = (cosf(SSFrameInfo::GetElapsedTime() * 3.0f) + 1.0f) * 0.5f;
		MeshColor.Z = (sinf(SSFrameInfo::GetElapsedTime() * 5.0f) + 1.0f) * 0.5f;
		MeshColor = Vector4f(1, 1, 1, 1);

//		Material->UpdateParameter(_deviceContext, 2, &MeshColor, sizeof(Vector4f));

		XMMATRIX Temp = RenderTarget->GetViewProjMatrix();

//		Material->UpdateTransform(_deviceContext, XMMatrixTranspose(XMMatrixRotationY(SSFrameInfo::GetElapsedTime())));

		Material->UpdateCameraSetting(_deviceContext, XMMatrixTranspose(RenderTarget->GetViewProjMatrix()));
	}


	uint32 IdxSize = Geometry->GetIndexDataNum();
	_deviceContext->DrawIndexed(IdxSize, 0, 0);


	SwapChain->Present(0, 0);
}


