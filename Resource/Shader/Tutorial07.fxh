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
    float4 vMeshColor; // �� ������ ��ȭ�� ��
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
    // TEXCOORD �ø�ƽ�� �ؽ��� ��ǥ �� �ƴ϶� ���е� �ִ� float���� ���ڸ� ������ ���� ���� ��
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
    
    float Diffuse = max(dot(input.Normal, LightDir), 0) * DIFFUSE_COEFF; // �븻�� �� ������ ���� Normal Light
    float Ambient = AMBIENT_COEFF;
    
    float color = Diffuse + Ambient;
    
    return float4(color, color, color, 1);
}
