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

cbuffer cbNeverChanges : register( b0 )
{
    matrix WVPTransform; // cbNeverChanges -> 절대 변하지 않는 콘스탄트 버퍼라는 뜻
};

cbuffer cbChangesEveryFrame : register( b1 )
{
    float4 vMeshColor; // 매 프레임 변화는 색
};


//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
    // TEXCOORD 시멘틱은 텍스쳐 좌표 뿐 아니라 정밀도 있는 float단위 숫자를 전달할 때도 많이 씀
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(input.Pos, WVPTransform);
    output.Tex = input.Tex;
    
    
    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PS_INPUT input) : SV_Target
{
    float4 color = txDiffuse.Sample(samLinear, input.Tex) * vMeshColor;
    return color;

//    return txDiffuse.Sample(samLinear, input.Tex) * vMeshColor;
    // samLinear라는 샘플링 기법
}
