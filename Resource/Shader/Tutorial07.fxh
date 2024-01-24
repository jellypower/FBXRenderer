

Texture2D txDiffuse : register(t0);

SamplerState samLinear : register(s0);

cbuffer ModelBuffer : register(b0)
{
    matrix WMatrix;
    matrix RotMatrix;
};

cbuffer CameraBuffer : register(b1)
{
    matrix VPMatrix;
};

cbuffer cbChangesEveryFrame : register(b2)
{
    float4 vMeshColor; // �� ������ ��ȭ�� ��
};


//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float4 Normal : NORMAL;
#ifndef NO_TANGENT_FRAME
    float4 Tangent : TANGENT;
#endif
    float2 UV0 : TEXCOORD0;
#ifndef NO_SECOND_UV
    float2 UV1 : TEXCOORD1;
#endif
#ifdef ENABLE_SKINNING
    uint4 jointIndices : BLENDINDICES;
    float4 jointWeights : BLENDWEIGHT;
#endif
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Normal : TEXCOORD0;
    float4 Color : TEXCOORD1;
    // TEXCOORD �ø�ƽ�� �ؽ��� ��ǥ �� �ƴ϶� ���е� �ִ� float���� ���ڸ� ������ ���� ���� ��
};


PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    output.Pos = mul(input.Pos, WMatrix);
    output.Pos = mul(output.Pos, VPMatrix);
    output.Normal = mul(input.Normal, RotMatrix);
    output.Color = input.Normal;
    
    return output;
}


float4 PS(PS_INPUT input) : SV_Target
{
    float3 LightDir = normalize(float3(-1, 1, 0));
    
    
    LightDir = normalize(LightDir);
    
    
    const float DIFFUSE_COEFF = 0.7;
    const float AMBIENT_COEFF = 0.3;
    
    float Diffuse = max(dot(input.Normal, LightDir), 0) * DIFFUSE_COEFF; // �븻�� �� ������ ���� Normal Light
    float Ambient = AMBIENT_COEFF;
    
    float color = Diffuse + Ambient;
    
    
    return float4(color, color, color, 1);
}
