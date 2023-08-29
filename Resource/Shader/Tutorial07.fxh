//--------------------------------------------------------------------------------------
// File: Tutorial07.fx
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register( t0 ); // ��ǻ����� 0�� �ؽ��� ������ �̿��Ѵٴ� ��

SamplerState samLinear : register( s0 ); 
// Sampler������Ʈ�� 0�� ���� �̿��Ѵ�.
// ���÷� ������Ʈ�� ������������, ���� �������� ���� �� �ִµ�

cbuffer cbNeverChanges : register( b0 )
{
    matrix WVPTransform; // cbNeverChanges -> ���� ������ �ʴ� �ܽ�źƮ ���۶�� ��
};

cbuffer cbChangesEveryFrame : register( b1 )
{
    float4 vMeshColor; // �� ������ ��ȭ�� ��
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
    // TEXCOORD �ø�ƽ�� �ؽ��� ��ǥ �� �ƴ϶� ���е� �ִ� float���� ���ڸ� ������ ���� ���� ��
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
    return txDiffuse.Sample(samLinear, input.Tex) * vMeshColor;
    // samLinear��� ���ø� ���
}
