#pragma once
#include <windows.h>
#include <d3d11.h>

#include "SSNativeTypes.h"

#include "SSShaderAsset.h"



enum class MaterialAssetInstanceStage {
	JustCreated = 0,
	Initialized = 1
};

class SSMaterial // Material�� ���� ����� �ƴ� �ν��Ͻ� �������� ����
{
public:

	// TODO: ���Ϸκ��� �о�ͼ� �ؽ���, ���̴� ��������
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


	// TODO: VertexInfoAsset ����� (�޽��� ���ؽ�, ���ؽ� ���� ������ ����ִ� ��)
	// TODO: MeshRenderer ����� (���ؽ��� ���̴��� ������ ���� �޽��� �׸��� ��)
};


inline void SSMaterial::UpdateWVPMatrix(ID3D11DeviceContext* InDeviceContext, const XMMATRIX& InMatrix)
{
	Shader->UpdateShaderWVPTransform(InDeviceContext, InMatrix);
}

