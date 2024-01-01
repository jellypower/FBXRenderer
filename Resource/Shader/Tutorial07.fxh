//--------------------------------------------------------------------------------------
// File: Tutorial07.fx
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register( t0 ); // 디퓨즈맵의 0번 텍스쳐 슬롯을 이용한다는 뜻

SamplerState samLinear : register( s0 ); 
// Sampler스테이트도 0번 슬롯 이용한다.
// 샘플러 스테이트로 선형보간일지, 비선형 보간일지 정할 수 있는듯

cbuffer ModelBuffer: register( b0 )
{
    matrix WMatrix; 
};

cbuffer CameraBuffer : register( b1 )
{
    matrix VPMatrix;  
};

cbuffer cbChangesEveryFrame : register( b2 )
{
    float4 vMeshColor; // 매 프레임 변화는 색
};


//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float4 Normal : NORMAL0;
    float4 Color : COLOR0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Normal : TEXCOORD0;
    float4 Color : TEXCOORD1;
    // TEXCOORD 시멘틱은 텍스쳐 좌표 뿐 아니라 정밀도 있는 float단위 숫자를 전달할 때도 많이 씀
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul( input.Pos, WMatrix );
    output.Pos = mul( output.Pos, VPMatrix );
    output.Normal = mul(input.Normal, WMatrix);
    output.Color = input.Normal;
    
    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 PS( PS_INPUT input) : SV_Target
{
    float3 LightDir = normalize(-float3(1, -1, 1));
    
    
    LightDir = normalize(LightDir);
    
    
    const float DIFFUSE_COEFF = 0.9;
    const float AMBIENT_COEFF = 0.1;
    
    float Diffuse = max(dot(input.Normal, LightDir), 0) * DIFFUSE_COEFF; // 노말과 빛 사이의 각도 Normal Light
    float Ambient = AMBIENT_COEFF;
    
    float color = Diffuse + Ambient;
    
    return float4(color, color, color, 1);
}
