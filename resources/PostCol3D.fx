float4x4 gWorldViewProj : WorldViewProjection;

Texture2D gDiffuseMap : DiffuseMap;
Texture2D gNormalMap : NormalMap;
Texture2D gSpecularMap : SpecularMap;
Texture2D gGlossinessMap : GlossinessMap;

float4x4  gWorldMatrix : WORLD;
float3    gCameraPosition : CAMERA;

static const float3 LightDirection = float3(0.577f, -0.577f, 0.577f);
static const float LightIntensity = float(7.0f);
static const float Shininess = float(25.0f);
static const float Ambient = float3(.025f, .025f, .025f);
static const float PI = float(3.14159265358979323846264338327950288f);

RasterizerState gRasterizerState
{
	Cullmode = back;
	FrontCounterClockwise = false;
};

BlendState gBlendState
{
	BlendEnable[0] = false;
};

DepthStencilState gDepthStencilState
{
	DepthEnable = true;
	DepthWriteMask = all;
	DepthFunc = less;
	StencilEnable = true;
};

SamplerState PointSampler
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = WRAP;
	AddressV = WRAP;
};

SamplerState LinearSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
};

SamplerState AnisotropicSampler
{
	Filter = ANISOTROPIC;
	AddressU = WRAP;
	AddressV = WRAP;
	MaxAnisotropy = 16;
};

// Input/Output structs
struct VS_INPUT {
	float3 Position : POSITION;
	float3 Color : COLOR;
	float2 TextureUV : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
};

struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float4 WorldPosition : WORLD;
	float4 Diffuse : COLOR;
	float2 TextureUV : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
};

// Vertex shader
VS_OUTPUT VS(VS_INPUT input) {
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.Position = mul(float4(input.Position, 1.f), gWorldViewProj);
	output.WorldPosition = mul(float4(input.Position, 1.f), gWorldMatrix);
	output.Diffuse = float4(input.Color, 1.0f);
	output.TextureUV = input.TextureUV;
	output.Normal = normalize(mul(normalize(input.Normal), (float3x3) gWorldMatrix));
	output.Tangent = normalize(mul(normalize(input.Tangent), (float3x3) gWorldMatrix));
	return output;
}

float3 LambertDiffuse(float4 color)
{
	return (LightIntensity * saturate(color)) / PI;
}

float3 CalculateNormal(float4 mapNormal, float3 tangent, float3 vertexNormal)
{
	float3 binormal = normalize(cross(vertexNormal, tangent));
	float3x3 tangentSpaceAxis = float3x3(tangent, binormal, vertexNormal);
	
	float3 sampledNormal = 2.f * mapNormal.rgb - float3(1.f, 1.f, 1.f);

	return normalize(mul(sampledNormal, tangentSpaceAxis));
}

float4 Shade(VS_OUTPUT input, float4 diffuse, float4 normal, float4 specular, float glossiness) {
	float3 finalNormal = CalculateNormal(normal, input.Tangent, input.Normal);
	
	float3 inverseViewDirection = normalize(gCameraPosition - input.WorldPosition.xyz);
	
	float3 lambertDiffuse = LambertDiffuse(diffuse);	
	float observedArea = max(dot(finalNormal, -LightDirection), 0.f);
	float3 reflection = reflect(LightDirection, finalNormal);
	
	float angle = max(dot(reflection, inverseViewDirection), 0.0f);
	
	float specReflection = pow(angle, glossiness);
	
	float3 phong = specReflection * specular;
	
	return float4((lambertDiffuse * observedArea) + phong, 1.f);
}

// Pixel shader for Point Sampling
float4 PS_Point(VS_OUTPUT input) : SV_TARGET
{
	float4 diffuse = gDiffuseMap.Sample(PointSampler, input.TextureUV) * input.Diffuse;
	float4 normal = gNormalMap.Sample(PointSampler, input.TextureUV);
	float4 specular = gSpecularMap.Sample(PointSampler, input.TextureUV);
	float glossiness = saturate(gGlossinessMap.Sample(PointSampler, input.TextureUV).r) * Shininess;
	return saturate(Shade(input, diffuse, normal, specular, glossiness) + Ambient);
}

// Pixel shader for Linear Sampling
float4 PS_Linear(VS_OUTPUT input) : SV_TARGET
{
	float4 diffuse = gDiffuseMap.Sample(LinearSampler, input.TextureUV) * input.Diffuse;
	float4 normal = gNormalMap.Sample(LinearSampler, input.TextureUV);
	float4 specular = gSpecularMap.Sample(LinearSampler, input.TextureUV);
	float glossiness = saturate(gGlossinessMap.Sample(LinearSampler, input.TextureUV).r) * Shininess;
	return saturate(Shade(input, diffuse, normal, specular, glossiness) + Ambient);
}

// Pixel shader for Anisotropic Sampling
float4 PS_Anisotropic(VS_OUTPUT input) : SV_TARGET
{
	float4 diffuse = gDiffuseMap.Sample(AnisotropicSampler, input.TextureUV) * input.Diffuse;
	float4 normal = gNormalMap.Sample(AnisotropicSampler, input.TextureUV);
	float4 specular = gSpecularMap.Sample(AnisotropicSampler, input.TextureUV);
	float glossiness = saturate(gGlossinessMap.Sample(AnisotropicSampler, input.TextureUV).r) * Shininess;
	return saturate(Shade(input, diffuse, normal, specular, glossiness) + Ambient);
}

// Techniques
technique11 PointTechnique{
	pass p0 {
		SetRasterizerState(gRasterizerState);
		SetDepthStencilState(gDepthStencilState, 0);
		SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS_Point()));
	}
}

technique11 LinearTechnique {
	pass p0
	{
		SetRasterizerState(gRasterizerState);
		SetDepthStencilState(gDepthStencilState, 0);
		SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS_Linear()));
	}
}

technique11 AnisotropicTechnique {
	pass p0
	{
		SetRasterizerState(gRasterizerState);
		SetDepthStencilState(gDepthStencilState, 0);
		SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS_Anisotropic()));
	}
}