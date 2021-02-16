#include "BasicStructures.hlsl"
#include "Math.hlsl"

struct VSInput
{
    float3 Pos      : ATTRIB0;
    float3 Normal   : ATTRIB1;
    float2 UV0      : ATTRIB2;
	float3 Tangent  : ATTRIB3;

	float4 World0	: ATTRIB4;
	float4 World1	: ATTRIB5;
	float4 World2 	: ATTRIB6;
	float4 World3 	: ATTRIB7;
};

cbuffer Globals
{
    RendererGlobalData mGlobals;
}

void main(in  VSInput VSIn,
    out float4 ClipPos  	: SV_Position,
	out float3 WorldPos 	: WORLD_POS,
    out float3 Normal    	: NORMAL,
	out float3 Tangent 	 	: TANGENT,
	out float2 UV	      	: UV0) 
{
    float4x4 Transform = MatrixFromRows(
		VSIn.World0, 
		VSIn.World1, 
		VSIn.World2, 
		VSIn.World3);

	float4 transformWorldPos = mul(Transform, float4(VSIn.Pos, 1.0));
    float3x3 NormalTransform = float3x3(Transform[0].xyz, Transform[1].xyz, Transform[2].xyz);
    NormalTransform = InverseTranspose3x3(NormalTransform);
    
	Normal = mul(NormalTransform, VSIn.Normal);
    Normal = Normal / max(length(Normal), 1e-5);

	Tangent = mul(NormalTransform, VSIn.Tangent);
	Tangent = Tangent / max(length(Tangent), 1e-5);

	WorldPos = transformWorldPos.xyz / transformWorldPos.w;
	ClipPos  = mul(float4(WorldPos, 1.0), mGlobals.mCamera.mViewProj);
    UV 	  = VSIn.UV0;
}
