
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
#include "RenderAsset/SSSkeletonAssetManager.h"
#include "RenderAsset/SSSkeletonAnimAssetManager.h"

#include "RenderAsset/AssetType/SSSkeletonAnimAsset.h"


#include "SSEngineDefault/SSContainer/SHasher.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include "SSSamplerPool.h"
#include "RenderAsset/SSModelCombinationAssetManager.h"

#include "RenderAsset/AssetType/SSMaterialAssetDetail/SSPbrMaterialAsset.h"



struct PbrMaterialParamEditorCopy
{
	PbrMaterialConstants pbrMaterialParam;
	SS::FixedStringA<ASSET_NAME_LEN_MAX> textureNames[SS_PBR_TX_COUNT];
};

void ExtractFileNameFromFilePath(SS::FixedStringA<ASSET_NAME_LEN_MAX>& OutFileName, const utf16* InFilePath)
{
	const utf16* fileNameStart = wcsrchr(InFilePath, '/');
	if (fileNameStart == nullptr)
		fileNameStart = wcsrchr(InFilePath, '\\');
	if (fileNameStart == nullptr)
	{
		OutFileName.Clear();
		return;
	}
	fileNameStart++;

	uint32 InFilePathLen = wcslen(fileNameStart);

	utf8 lFileName[ASSET_NAME_LEN_MAX];
	SS::UTF16StrToUtf8Str(fileNameStart, InFilePathLen, lFileName, ASSET_NAME_LEN_MAX);

	OutFileName = lFileName;
	uint32 cutOutLen = strrchr(OutFileName, '.') - OutFileName;
	OutFileName.CutOut(cutOutLen);


}

static D3D_PRIMITIVE_TOPOLOGY ConvertToD3DTopology(EGeometryDrawTopology InTopology) {
	switch (InTopology) {

	case EGeometryDrawTopology::None:
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	case EGeometryDrawTopology::TriangleList:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case EGeometryDrawTopology::PointList:
		return D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;


	default:
		assert(false);
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}


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
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;

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
		sd.BufferCount = 1;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

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

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(_d3DDevice, _deviceContext);


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

	SSSkeletonAssetManager::Instantiate(100, 1000, 10, RANDOM_PRIMENO_FOR_HASH);
	SSSkeletonAnimAssetManager::Instantiate(100, 1000, 10, RANDOM_PRIMENO_FOR_HASH);

	SSGeometryAssetManager::Instantiate(1000, 1000, 10, RANDOM_PRIMENO_FOR_HASH);
	SSModelAssetManager::Instantiate(1000, 1000, 10, RANDOM_PRIMENO_FOR_HASH);
	SSModelCombinationAssetManager::Instantiate(100, 1000, 10, RANDOM_PRIMENO_FOR_HASH);



	hr = ImportFBXFileToAssetPool();
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG();
		return hr;
	}


	SSMaterialAssetManager::Get()->InstantiateAllMaterials(_d3DDevice, _deviceContext);

	SSSkeletonAssetManager::Get()->InstantiateAllSkeletonGPUBuffer(_d3DDevice, _deviceContext);
	SSSkeletonAnimAssetManager::Get()->InstantiateAllAsets(_d3DDevice, _deviceContext);

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

void SSRenderer::BindFbxFilePathToImport(const utf16* FilePath)
{
	_fbxFilePath = FilePath;
}


HRESULT SSRenderer::ImportFBXFileToAssetPool()
{
	HRESULT hr = S_OK;

	utf8 fbxFilePathA[PATH_LEN_MAX];


	SS::UTF16StrToUtf8Str(L"Resource\\DefaultMesh\\DirectionMesh.fbx", 39, fbxFilePathA, PATH_LEN_MAX);
	hr = _fbxImporter.LoadModelAssetFromFBXFile(fbxFilePathA);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG();
		return hr;
	}
	_fbxImporter.StoreCurrentFBXModelAssetToAssetManager();
	_fbxImporter.ClearFBXModelAsset();

	SS::UTF16StrToUtf8Str(_fbxFilePath, _fbxFilePath.GetLen(), fbxFilePathA, PATH_LEN_MAX);
	hr = _fbxImporter.LoadModelAssetFromFBXFile(fbxFilePathA);
	if (FAILED(hr)) {
		SS_CLASS_ERR_LOG();
		return hr;
	}
	_fbxImporter.StoreCurrentFBXModelAssetToAssetManager();
	_fbxImporter.ClearFBXModelAsset();




	_pbrMaterialParamcopyList = DBG_NEW PbrMaterialParamEditorCopy[SSMaterialAssetManager::GetAssetCount()];
	for (uint32 i = 0; i < SSMaterialAssetManager::GetAssetCount(); i++)
	{
		SSMaterialAsset* thisMaterialAsset = SSMaterialAssetManager::GetAssetWithIdx(i);
		if (thisMaterialAsset->GetExplicitMaterialType() == ExplicitMaterialType::SSDefaultPbrMaterial)
		{
			const SSPbrMaterialAsset* thisPbrMaterial = static_cast<SSPbrMaterialAsset*>(thisMaterialAsset);
			_pbrMaterialParamcopyList[i].pbrMaterialParam = thisPbrMaterial->GetMaterialParam();

			_pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_BASE_COLOR_IDX] = thisPbrMaterial->GetTextureName(SS_PBR_TX_BASE_COLOR_IDX);
			_pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_NORMAL_IDX] = thisPbrMaterial->GetTextureName(SS_PBR_TX_NORMAL_IDX);
			_pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_METALLIC_IDX] = thisPbrMaterial->GetTextureName(SS_PBR_TX_METALLIC_IDX);
			_pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_EMISSIVE_IDX] = thisPbrMaterial->GetTextureName(SS_PBR_TX_EMISSIVE_IDX);
			_pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_OCCLUSION_IDX] = thisPbrMaterial->GetTextureName(SS_PBR_TX_OCCLUSION_IDX);
		}
	}


	return hr;
}


void SSRenderer::CleanUp()
{
	SSModelCombinationAssetManager::Get()->ReleaseAllAssets();
	SSModelCombinationAssetManager::Release();

	SSModelAssetManager::Get()->ReleaseAllModels();
	SSModelAssetManager::Release();

	SSGeometryAssetManager::Get()->ReleaseAllGeometryDataOnGPU();
	SSGeometryAssetManager::Release();

	delete _pbrMaterialParamcopyList;

	SSSkeletonAnimAssetManager::Get()->ReleaseAllAssets();
	SSSkeletonAnimAssetManager::Release();

	SSSkeletonAssetManager::Get()->ReleaseAllSkeletons();
	SSSkeletonAssetManager::Get()->Release();


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

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

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

	SS::SHashA::ClearHashPool();
}

void SSRenderer::BindModel(const SSModelAsset* modelToBind, uint8 multiMaterialIdx)
{
	if (multiMaterialIdx >= modelToBind->GetMultiMaterialCount())
	{
		SS_CLASS_WARNING_LOG("Material idx invalid(Material Idx Max/idx to draw : %d/%d)", multiMaterialIdx, modelToBind->GetMultiMaterialCount());
		return;
	}

	if (modelToBind->GetMaterial(multiMaterialIdx)->IsBindingPossible()) BindMaterial(modelToBind->GetMaterial(multiMaterialIdx));
	else {
		SS_CLASS_ERR_LOG("material of Asset is not bindable");
		return;
	}

	if (modelToBind->GetGeometry()->UsableOnGPU()) BindGeometry(modelToBind->GetGeometry());
	else {
		SS_CLASS_ERR_LOG("material of Asset is not bindable");
		return;
	}

}

void SSRenderer::BindSkeleton(SSSkeletonAsset* skeletonToBind)
{
	_deviceContext->VSSetShaderResources(3, 1, skeletonToBind->GetJointBufferSRVPtr());
}

void SSRenderer::BindSkeletonAnim(SSSkeletonAnimAsset* skeletonAnimAsset, float time)
{
	skeletonAnimAsset->UpdateGPUBufferFrameState(_deviceContext, time);
//	skeletonAnimAsset->ResetJointBufferState(_deviceContext);
	_deviceContext->VSSetShaderResources(4, 1, skeletonAnimAsset->GetJointBufferSRVPtr());
}


void SSRenderer::BindMaterial(SSMaterialAsset* materialAsset)
{
	if (materialAsset->GetMaterialStage() < MaterialAssetInstanceStage::SystemBufferInitialized) {
		SS_CLASS_ERR_LOG("To bind material, Material must be initialized\n");
		return;
	}
	BindShader(materialAsset->GetShader());


	uint8 VSBufferCnt = materialAsset->GetVSConstantBufferCnt(); // TODO: Hack
	if (strcmp(materialAsset->GetShader()->GetAssetName(), SSShaderAssetManager::SSDefaultPbrSkinnedShaderName) == 0)
		VSBufferCnt -= 2;

	for (int i = 0; i < VSBufferCnt; i++) {
		_deviceContext->VSSetConstantBuffers(materialAsset->GetVSConstantBufferIdx(i), 1,
			materialAsset->GetVSConstantBufferPtr(i));
	}
	for (int i = 0; i < materialAsset->GetPSConstantBufferCnt(); i++) {
		_deviceContext->PSSetConstantBuffers(materialAsset->GetPSConstantBufferIdx(i), 1,
			materialAsset->GetPSConstantBufferPtr(i));
	}

	for (uint32 i = 0; i < materialAsset->GetTextureCnt(); i++)
	{

		_deviceContext->PSSetShaderResources(i, 1, materialAsset->GetBoundTextureAsset(i)->GetSRVpp());
	}


	_deviceContext->PSSetSamplers(0, materialAsset->GetSamplerCnt(), materialAsset->GetSamplerPtr());

}

void SSRenderer::BindGeometry(SSGeometryAsset* geometryAsset)
{
	uint32 offset = 0;
	uint32 stride = geometryAsset->GetEachVertexDataSize();
	_deviceContext->IASetVertexBuffers(0, 1, geometryAsset->GetVertexBufferPtr(), &stride, &offset);
	_deviceContext->IASetIndexBuffer(geometryAsset->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	_deviceContext->IASetPrimitiveTopology(ConvertToD3DTopology(geometryAsset->GetDrawTopology()));
}

void SSRenderer::BindShader(SSShaderAsset* shaderAsset)
{

	if (shaderAsset->GetShaderInstanceStage() < ShaderAssetInstanceStage::Instantiated) {
		WSS_LOG(L"Warning[%ls]: Shader is not initalized completely. CurState: %d\n"
			, shaderAsset->GetAssetPath(), shaderAsset->GetShaderInstanceStage());
		return;
	}

	_deviceContext->IASetInputLayout(shaderAsset->GetInputLayout());
	_deviceContext->VSSetShader(shaderAsset->GetVertexShader(), nullptr, 0);
	_deviceContext->PSSetShader(shaderAsset->GetPixelShader(), nullptr, 0);
}

void SSRenderer::SetModelTransform(Transform boundAssetTransform)
{

}

void SSRenderer::Draw()
{

}

void SSRenderer::BeginFrame()
{
	_renderTarget = DBG_NEW SSCamera();
	_renderTarget->UpdateResolutionWithClientRect(_d3DDevice, hWnd);
	_renderTarget->GetTransform().Position = Vector4f(.0f, .0f, -5.0f, .0f);
	_renderTarget->GetTransform().Rotation = Quaternion::FromLookDirect(Vector4f::Zero - _renderTarget->GetTransform().Position);
	_renderTarget->SetFOVWithRadians(XM_PIDIV4);
	_renderTarget->SetNearFarZ(0.01f, 10000.f);

	SS_LOG("Log (SSRenderer): Renderer Init finished!\n");

	_globalParamContext.SunDirection = _renderTarget->GetTransform().GetBackward();
	_globalParamContext.SunIntensity = Vector4f::One * 1.5;

	ExtractFileNameFromFilePath(_mdlcAssetName, _fbxFilePath);
	_mdlcAssetName += ".mdlc";
	_mdlcCache = SSModelCombinationAssetManager::FindAssetWithName(_mdlcAssetName);

	_skeletonAssetCache = SSSkeletonAssetManager::FindAssetWithName("rp_nathan_animated_003_walking.skl");
	_skeletonAnimCache = SSSkeletonAnimAssetManager::Get()->FindAssetWithName("rp_nathan_animated_003_walking.ska");
	_directionMeshCache = SSModelAssetManager::FindModelWithName("DirectionMesh_Direction.mdl");

}

void SSRenderer::PerFrame()
{
	_deviceContext->ClearRenderTargetView(RenderTargetView, Colors::MidnightBlue);
	_deviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Imgui settings
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Material List");
		{
			for (uint32 i = 0; i < SSMaterialAssetManager::GetAssetCount(); i++)
			{
				SSMaterialAsset* thisMaterial = SSMaterialAssetManager::GetAssetWithIdx(i);

				if (ImGui::CollapsingHeader(thisMaterial->GetAssetName()))
				{
					ImGui::Text("Material Type: %s", ExplicitMaterialTypeStr[(uint32)thisMaterial->GetExplicitMaterialType()]);
					ImGui::Text("Shader Name: %s", thisMaterial->GetShader()->GetAssetName());

					if (thisMaterial->GetExplicitMaterialType() == ExplicitMaterialType::SSDefaultPbrMaterial)
					{
						SSPbrMaterialAsset* thisPbrMaterial = static_cast<SSPbrMaterialAsset*>(thisMaterial);
						ImGui::BeginChild(thisMaterial->GetAssetName(), ImVec2(0, 0), ImGuiChildFlags_AutoResizeY);

						ImGui::TextColored(ImVec4(1, 1, 0, 1), "Parameters: ");

						ImGui::ColorEdit4("BaseColorFactor", (float*)&(_pbrMaterialParamcopyList[i].pbrMaterialParam.baseColorFactor));
						ImGui::ColorEdit4("EmissiveColorFactor", (float*)&(_pbrMaterialParamcopyList[i].pbrMaterialParam.emissiveFactor));
						ImGui::SliderFloat("MetallicFactor", &(_pbrMaterialParamcopyList[i].pbrMaterialParam.metallicFactor), 0.0f, 1.0f);
						ImGui::SliderFloat("RoughnessFactor", &(_pbrMaterialParamcopyList[i].pbrMaterialParam.roughnessFactor), 0.0f, 1.0f);


						if (ImGui::Button("Sync Material Param"))
						{
							thisPbrMaterial->SetMaterialParam(_pbrMaterialParamcopyList[i].pbrMaterialParam);
							thisPbrMaterial->SyncAllGPUBuffer(_deviceContext);
						}



						if (ImGui::TreeNode("Material Textures"))
						{
							if (ImGui::BeginCombo("Diffuse Texture", _pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_BASE_COLOR_IDX]))
							{
								SS::FixedStringA<ASSET_NAME_LEN_MAX>& selectedTextureName = _pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_BASE_COLOR_IDX];
								for (uint32 j = 0; j < SSTextureAssetManager::GetAssetCount(); j++)
								{
									SSTextureAsset* comboBoxTextureItem = SSTextureAssetManager::GetAssetWithIdx(j);
									bool isSelected = false;
									if (strcmp(selectedTextureName, comboBoxTextureItem->GetAssetName()) == 0) isSelected = true;

									if (ImGui::Selectable(comboBoxTextureItem->GetAssetName(), isSelected))
										selectedTextureName = comboBoxTextureItem->GetAssetName();
									if (isSelected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}
							if (ImGui::BeginCombo("Normal Texture", _pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_NORMAL_IDX]))
							{
								SS::FixedStringA<ASSET_NAME_LEN_MAX>& selectedTextureName = _pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_NORMAL_IDX];
								for (uint32 j = 0; j < SSTextureAssetManager::GetAssetCount(); j++)
								{
									SSTextureAsset* comboBoxTextureItem = SSTextureAssetManager::GetAssetWithIdx(j);
									bool isSelected = false;
									if (strcmp(selectedTextureName, comboBoxTextureItem->GetAssetName()) == 0) isSelected = true;

									if (ImGui::Selectable(comboBoxTextureItem->GetAssetName(), isSelected))
										selectedTextureName = comboBoxTextureItem->GetAssetName();
									if (isSelected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}
							if (ImGui::BeginCombo("Metallic Texture", _pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_METALLIC_IDX]))
							{
								SS::FixedStringA<ASSET_NAME_LEN_MAX>& selectedTextureName = _pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_METALLIC_IDX];
								for (uint32 j = 0; j < SSTextureAssetManager::GetAssetCount(); j++)
								{
									SSTextureAsset* comboBoxTextureItem = SSTextureAssetManager::GetAssetWithIdx(j);
									bool isSelected = false;
									if (strcmp(selectedTextureName, comboBoxTextureItem->GetAssetName()) == 0) isSelected = true;

									if (ImGui::Selectable(comboBoxTextureItem->GetAssetName(), isSelected))
										selectedTextureName = comboBoxTextureItem->GetAssetName();
									if (isSelected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}
							if (ImGui::BeginCombo("EMissive Texture", _pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_EMISSIVE_IDX]))
							{
								SS::FixedStringA<ASSET_NAME_LEN_MAX>& selectedTextureName = _pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_EMISSIVE_IDX];
								for (uint32 j = 0; j < SSTextureAssetManager::GetAssetCount(); j++)
								{
									SSTextureAsset* comboBoxTextureItem = SSTextureAssetManager::GetAssetWithIdx(j);
									bool isSelected = false;
									if (strcmp(selectedTextureName, comboBoxTextureItem->GetAssetName()) == 0) isSelected = true;

									if (ImGui::Selectable(comboBoxTextureItem->GetAssetName(), isSelected))
										selectedTextureName = comboBoxTextureItem->GetAssetName();
									if (isSelected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}
							if (ImGui::BeginCombo("Occlusion Texture", _pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_OCCLUSION_IDX]))
							{
								SS::FixedStringA<ASSET_NAME_LEN_MAX>& selectedTextureName = _pbrMaterialParamcopyList[i].textureNames[SS_PBR_TX_OCCLUSION_IDX];
								for (uint32 j = 0; j < SSTextureAssetManager::GetAssetCount(); j++)
								{
									SSTextureAsset* comboBoxTextureItem = SSTextureAssetManager::GetAssetWithIdx(j);
									bool isSelected = false;
									if (strcmp(selectedTextureName, comboBoxTextureItem->GetAssetName()) == 0) isSelected = true;

									if (ImGui::Selectable(comboBoxTextureItem->GetAssetName(), isSelected))
										selectedTextureName = comboBoxTextureItem->GetAssetName();
									if (isSelected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}

							if (ImGui::Button("Sync Texture"))
							{
								for (uint32 j = 0; j < SS_PBR_TX_COUNT; j++)
									thisPbrMaterial->SetPBRTextureName(_pbrMaterialParamcopyList[i].textureNames[j], (SS_PBR_TEXTURE_IDX)j);
								thisPbrMaterial->SyncAllTextureItemWithName();
							}

							ImGui::TreePop();
						}


						ImGui::Spacing();
						ImGui::EndChild();
					}
				}
			}


		}
		ImGui::End();

		ImGui::Begin("Texture List");
		{
			if (ImGui::BeginTable("Textures", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders))
			{
				ImGui::TableNextColumn();
				ImGui::TextColored(ImVec4(1, 1, 0, 1), "Texture Name");
				ImGui::TableNextColumn();
				ImGui::TextColored(ImVec4(1, 1, 0, 1), "Texture Path");

				for (uint32 i = 0; i < SSTextureAssetManager::GetAssetCount(); i++)
				{
					SSTextureAsset* thisAsset = SSTextureAssetManager::GetAssetWithIdx(i);

					ImGui::TableNextColumn();
					ImGui::Text(thisAsset->GetAssetName());

					ImGui::TableNextColumn();
					utf8 assetPathUtf8[PATH_LEN_MAX];
					SS::UTF16StrToUtf8Str(thisAsset->GetAssetPath(), thisAsset->GetAssetPathLen(), assetPathUtf8, PATH_LEN_MAX);
					ImGui::Text(assetPathUtf8);
				}

				ImGui::EndTable();
			}

		}
		ImGui::End();

		ImGui::Begin("Global Settings");
		{
			ImGui::SliderFloat4("Light Intensity", (float*)&(_globalParamContext.SunIntensity), 0, 3);

		}
		ImGui::End();


		ImGui::Begin("Frame Info");
		{
			ImGui::Text("Elapsed time: %f", SSFrameInfo::GetElapsedTime());
			ImGui::Text("Delta time: %f", SSFrameInfo::GetDeltaTime());
			ImGui::Text("FPS: %f", SSFrameInfo::GetFPS());
		}
		ImGui::End();
	}

	// mySTL Test
	{

		for (uint32 i = 0; i < 2048; i++)
		{
			char str[10];
			itoa(i, str, 10);
			SS::SHashA Value(str);
		}

		SS::SHashA Value("1622");


		SS::SHashA Hash1("1586");
		SS::SHashA Hash2("1586");
		SS::SHashA Hash3("17");
		SS::SHashA Hash4("17");

		bool result1 = Hash1 == Hash2;
		bool result2 = Hash4 == Hash3;
		bool result3 = Hash1 == Hash1;
		bool result4 = Hash4 == Hash1;

		


		int a = 10;
	}

	constexpr float CAM_XROT_MAX = 0.9;
	constexpr float CAM_ROT_SPEED = 2;
	constexpr float CAM_SPEED = 3;
	constexpr float KEYBOARD_CAM_ROT_SPEED = 0.5;
	constexpr float CAM_ZOOM_SPEED = 1;


	// process mouse input
	if (SSInput::GetMouse(EMouseCode::MOUSE_RIGHT))
	{


		_camYRotation += SSInput::GetMouseDelta().X * CAM_ROT_SPEED;

		if (_camXRotation > -XM_PIDIV2 * CAM_XROT_MAX && SSInput::GetMouseDelta().Y > 0)
			_camXRotation -= SSInput::GetMouseDelta().Y * CAM_ROT_SPEED;

		if (_camXRotation < XM_PIDIV2 * CAM_XROT_MAX && SSInput::GetMouseDelta().Y < 0)
			_camXRotation -= SSInput::GetMouseDelta().Y * CAM_ROT_SPEED;

	}

	// process keyborad input
	{
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
			_camYRotation += SSFrameInfo::GetDeltaTime() * KEYBOARD_CAM_ROT_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_LEFT))
			_camYRotation -= SSFrameInfo::GetDeltaTime() * KEYBOARD_CAM_ROT_SPEED;

		if (SSInput::GetKey(EKeyCode::KEY_UP))
		{
			if (_camXRotation > -XM_PIDIV2 * CAM_XROT_MAX)
				_camXRotation -= SSFrameInfo::GetDeltaTime() * KEYBOARD_CAM_ROT_SPEED;
		}

		if (SSInput::GetKey(EKeyCode::KEY_DOWN))
		{
			if (_camXRotation < XM_PIDIV2 * CAM_XROT_MAX)
				_camXRotation += SSFrameInfo::GetDeltaTime() * KEYBOARD_CAM_ROT_SPEED;
		}
	}


	_camYRotation = fmodf(_camYRotation, XM_2PI);
	_renderTarget->GetTransform().Rotation = XMQuaternionRotationRollPitchYaw(_camXRotation, _camYRotation, 0);
	_globalParamContext.VPMatrix = XMMatrixTranspose(_renderTarget->GetViewProjMatrix());
	_globalParamContext.ViewerPos = _renderTarget->GetTransform().Position;
	_globalParamContext.SunDirection = _renderTarget->GetTransform().GetBackward();


	Transform drawTransform;

	uint32 frame = (uint32)(SSFrameInfo::GetElapsedTime() * 24) % 55;


	if (_skeletonAssetCache)
	{
		DrawAnimatedSkeletonRecursion(_skeletonAssetCache->GetBones(), _skeletonAnimCache->GetAnimStack().GetData() + 88 * frame, 0, drawTransform);
	}


	TraverseModelCombinationAndDraw(_mdlcCache, drawTransform.AsMatrix(), drawTransform.Rotation.AsMatrix());

	{
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
	SwapChain->Present(0, 0);
}

void SSRenderer::TraverseModelCombinationAndDraw(SSPlaceableAsset* asset, XMMATRIX transformMatrix, XMMATRIX rotMatrix)
{
	if (asset->GetAssetType() == AssetType::ModelCombination && asset->GetParent() != nullptr)
	{
		const SSModelCombinationAsset* modelCombination = static_cast<const SSModelCombinationAsset*>(asset);
		const SSModelAsset* modelAsset = modelCombination->GetModelAsset();

		for (uint8 i = 0; i < modelAsset->GetMultiMaterialCount(); i++)
		{
			BindModel(modelAsset, i);

			if (modelAsset->GetGeometry()->GetMeshType() == EMeshType::Skinned)
			{
				BindSkeleton(_skeletonAssetCache);
				BindSkeletonAnim(_skeletonAnimCache, SSFrameInfo::GetElapsedTime() / 2);
			}

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
		TraverseModelCombinationAndDraw(item,
			item->GetTransform().AsMatrix() * transformMatrix,
			item->GetTransform().Rotation.AsMatrix() * rotMatrix);
	}

}

void SSRenderer::TraverseSkeletonAndDrawRecursion(const SS::PooledList<BoneNode>& boneList, const uint16 boneIdx, XMMATRIX transformMatrix, XMMATRIX rotMatrix)
{
	BindModel(_directionMeshCache);
	_directionMeshCache->GetMaterial()->UpdateGlobalRenderParam(_deviceContext, _globalParamContext);

	_directionMeshCache->GetMaterial()->UpdateTransform(_deviceContext, transformMatrix, rotMatrix);
	_deviceContext->DrawIndexed(_directionMeshCache->GetGeometry()->GetIndexDataNum(), _directionMeshCache->GetGeometry()->GetIndexDataStartIndex(), 0);

	for (const uint16 childIdx : boneList[boneIdx]._childs)
	{
		TraverseSkeletonAndDrawRecursion(boneList, childIdx,
			boneList[childIdx]._transform.AsMatrix() * transformMatrix,
			boneList[childIdx]._transform.Rotation.AsMatrix() * rotMatrix);
	}
}

void SSRenderer::DrawAnimatedSkeleton(const SS::PooledList<BoneNode>& boneList, const float frameTime, const uint16 boneIdx, Transform transform)
{
	
}

void SSRenderer::DrawAnimatedSkeletonRecursion(const SS::PooledList<BoneNode>& boneList, const Transform* animTransform, const uint16 boneIdx, Transform transform)
{
	BindModel(_directionMeshCache);
	_directionMeshCache->GetMaterial()->UpdateGlobalRenderParam(_deviceContext, _globalParamContext);

	Transform currentTransform = animTransform[boneIdx] * transform;
	_directionMeshCache->GetMaterial()->UpdateTransform(_deviceContext, currentTransform.AsMatrix(), currentTransform.Rotation.AsMatrix());
	_deviceContext->DrawIndexed(_directionMeshCache->GetGeometry()->GetIndexDataNum(), _directionMeshCache->GetGeometry()->GetIndexDataStartIndex(), 0);


	for (const uint16 childIdx : boneList[boneIdx]._childs)
	{
		DrawAnimatedSkeletonRecursion(boneList, animTransform, childIdx,
			currentTransform);
	}
}