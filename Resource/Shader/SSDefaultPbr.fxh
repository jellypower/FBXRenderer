
Texture2D txBaseColor                       : register(t0);
Texture2D<float3> txMetallic                : register(t1);
Texture2D<float3> txEmissive                : register(t2);
Texture2D<float3> txNormal                  : register(t3);




SamplerState samLinear : register(s0);

cbuffer ModelBuffer : register(b0)
{
    matrix WMatrix;
    matrix RotMatrix;
};

cbuffer GlobalRenderParam : register(b1)
{
    matrix VPMatrix;
    float3 SunDirection;
};


cbuffer MaterialParam : register(b2)
{
    float4 baseColorFactor;
    float3 emissiveFactor;
    float normalTextureScale;
    float2 metallicRoughnessFactor;
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
    float3 Normal : NORMAL;
    
#ifndef NO_TANGENT_FRAME
    float4 Tangent : TANGENT;
#endif

    float2 UV0 : TEXCOORD0;
#ifndef NO_SECOND_UV
    float2 UV1 : TEXCOORD1;
#endif

    float3 WorldPos : TEXCOORD2;
};


PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    output.Pos = mul(input.Pos, WMatrix);
    output.Pos = mul(output.Pos, VPMatrix);
    output.Normal = mul(input.Normal, RotMatrix);
#ifndef NO_TANGENT_FRAME
    output.Tangent = mul(RotMatrix, input.Tangent.xyz); // TODO: tangent 관련 문제 생길수도 있음
#endif

    output.UV0 = input.UV0;
    output.UV1 = input.UV1;
    output.WorldPos = mul(input.Pos, WMatrix);
    
    return output;
}


// =====================================================================================================


float4 PS(PS_INPUT input) : SV_Target
{
    float4 baseColor = baseColorFactor * txBaseColor.Sample(samLinear, 0);
    float2 metallic = metallicRoughnessFactor * txMetallic.Sample(samLinear, 0).bg;
    float3 emissive = emissiveFactor * txEmissive.Sample(samLinear, 0);

    float3 LightDir = normalize(SunDirection);

    const float DIFFUSE_COEFF = 0.7;
    const float AMBIENT_COEFF = 0.3;
    
    float4 Diffuse = txBaseColor.Sample(samLinear, input.UV0) * max(dot(input.Normal, LightDir), 0) * DIFFUSE_COEFF; // 노말과 빛 사이의 각도 Normal Light
    float Ambient = AMBIENT_COEFF;
    
    float4 color = Diffuse + Ambient;

    return color;
}
