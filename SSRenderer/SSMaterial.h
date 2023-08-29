#pragma once
#include <windows.h>
#include <d3d11.h>

#include "SSNativeTypes.h"

#include "SSShaderAsset.h"



enum class MaterialAssetInstanceStage {
	JustCreated = 0,
	Initialized = 1
};

class SSMaterial // Material은 에셋 계념이 아닌 인스턴스 개념으로 가자
{
public:

	// TODO: 파일로부터 읽어와서 텍스쳐, 셰이더 가져오기
	HRESULT InitTemp(ID3D11Device* InDevice,
		class SSShaderAsset* InShaderAsset, class SSTextureManager* InTextureManager);
	void Release();

	void BindMaterial(ID3D11DeviceContext* InDeviceContext);
	bool IsBindingPossible() { return MaterialStage >= MaterialAssetInstanceStage::Initialized; }

	void UpdateParameter(int idx, const void* InData, uint32 InDataSize);
	

	__forceinline void UpdateWVPMatrix(ID3D11DeviceContext* InDeviceContext, const XMMATRIX& InMatrix);

	__forceinline MaterialAssetInstanceStage GetMaterialStage() const { return MaterialStage; }

protected:
	void* ConstBufferData[CONSTANT_BUFFER_COUNT_MAX] = { 0, };

private:


	class SSShaderAsset* Shader = nullptr;
	
	const SSShaderReflectionForMaterial* ShaderReflectionMetadata;
	
	ID3D11ShaderResourceView** TextureList = nullptr;

	ID3D11SamplerState** SampleStateList = nullptr;

	MaterialAssetInstanceStage MaterialStage = MaterialAssetInstanceStage::JustCreated;


	// TODO: VertexInfoAsset 만들기 (메쉬의 버텍스, 버텍스 관련 정보만 담고있는 놈)
	// TODO: MeshRenderer 만들기 (버텍스와 쉐이더를 가지고 실제 메쉬를 그리는 놈)
};


inline void SSMaterial::UpdateWVPMatrix(ID3D11DeviceContext* InDeviceContext, const XMMATRIX& InMatrix)
{
	Shader->UpdateShaderWVPTransform(InDeviceContext, InMatrix);
}

