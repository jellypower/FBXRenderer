
Texture2D txBaseColor : register(t0);
Texture2D<float3> txNormal : register(t1);
Texture2D<float3> txMetallic : register(t2);
Texture2D txEmissive : register(t3);
Texture2D<float> txOcclusion : register(t4);




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
    float3 SunIntensity;
    float3 ViewerPos;
};


cbuffer MaterialParam : register(b2)
{
    float4 baseColorFactor;
    float4 emissiveFactor;
    float normalTextureScale;
    float2 metallicRoughnessFactor;
};


//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float4 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV0 : TEXCOORD0;
    float2 UV1 : TEXCOORD1;
#ifdef ENABLE_SKINNING
    uint4 jointIndices : BLENDINDICES;
    float4 jointWeights : BLENDWEIGHT;
#endif
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV0 : TEXCOORD0;
    float2 UV1 : TEXCOORD1;
    float3 WorldPos : TEXCOORD2;
};


PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    output.Pos = mul(input.Pos, WMatrix);
    output.Pos = mul(output.Pos, VPMatrix);
    output.Normal = mul(input.Normal, RotMatrix);
    output.Tangent = mul(input.Tangent, RotMatrix);

    output.UV0 = input.UV0;
    output.UV1 = input.UV1;
    output.WorldPos = mul(input.Pos, WMatrix);
    
    return output;
}


// =====================================================================================================
static const float3 kDielectricSpecular = float3(0.04, 0.04, 0.04);
// 유전체(dielectric material)는 전기장 안에서 극성을 지니게 되는 절연체이다.

float3 ComputeNormal(PS_INPUT psInput)
{
    float3 normal = normalize(psInput.Normal);
    float3 tangent = normalize(psInput.Tangent.xyz);
    float3 bitangent = normalize(cross(normal, tangent)) * psInput.Tangent.w;

	float3x3 tangentFrame = float3x3(tangent, bitangent, normal);
    
    normal = txNormal.Sample(samLinear, psInput.UV0) * 2.0 - 1.0;
    normal = normalize(normal);
    normal = normalize(normal * float3(normalTextureScale, normalTextureScale, 1));

    return mul(normal, tangentFrame);
}

float Pow5(float x)
{
    float xSq = x * x;
    return xSq * xSq * x;
}

// Shlick's approximation of Fresnel
float3 Fresnel_Shlick(float3 F0, float3 F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}

float Fresnel_Shlick(float F0, float F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}


float3 ComputeDiffuse(float3 Diffuse, float3 N, float V, float NdotV,float Roughness)
{
    float LdotH = saturate(dot(N, normalize(N + V)));


    float fd90 = 0.5 + 2.0 * Roughness * LdotH * LdotH;
    
    float3 DiffuseBurley = Diffuse * Fresnel_Shlick(1, fd90, NdotV);

    return DiffuseBurley;
}

float3 ComputeSpecular(float3 Specular, float3 N, float3 V)
{
    
    float NdotV = saturate(dot(N, V));
    float3 specular = Fresnel_Shlick(Specular, 1, NdotV);
    
	return specular;

}

float4 PS(PS_INPUT input) : SV_Target
{
    float4 baseColor = baseColorFactor * txBaseColor.Sample(samLinear, input.UV0);
    float2 metallic = metallicRoughnessFactor * txMetallic.Sample(samLinear, input.UV0).bg;
    float occlusion = txOcclusion.Sample(samLinear, input.UV0);

    float4 emissiveSample = txEmissive.Sample(samLinear, input.UV0);
    float3 emissive = emissiveFactor * emissiveSample.rgb * emissiveSample.a;

    float3 LightDir = normalize(SunDirection);

    const float DIFFUSE_COEFF = 0.7;
    const float AMBIENT_COEFF = 0.3;

    float3 normal = ComputeNormal(input);
    float3 view = normalize(ViewerPos - input.WorldPos);
    float3 NdotV = saturate(dot(normal, view));
    float3 diffuseColor = baseColor.rgb * (1 - kDielectricSpecular) * (1 - metallic.x) * occlusion;
    float3 specularColor = lerp(kDielectricSpecular, baseColor.rgb, metallic.x) * occlusion;
    float roughness = metallic.y;
    float alpha = metallic.y * metallic.y;
    float alphaSqr = alpha * alpha;

    float3 colorAccum = emissive;

    diffuseColor = ComputeDiffuse(diffuseColor, normal, view, NdotV, roughness);
    specularColor = ComputeSpecular(specularColor, normal, view);

    colorAccum += diffuseColor;
    colorAccum += specularColor;
    
    float4 color = float4(colorAccum, baseColor.a);


    // HACK: 
    if (color.x != 12.487489)
    {
//        color = float4(diffuseColor + emissive, baseColor.a);
        color = float4(specularColor, baseColor.a);

        /*
        float3 Specular;
        float3 Half = normalize(LightDir + view);
        float HalfDot = saturate(dot(Half, normal));
        Specular = pow(HalfDot, 0.5);

        color = float4(Specular, 1);*/

    }

    return color;
}
