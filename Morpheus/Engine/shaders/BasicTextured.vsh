#include "BasicStructures.hlsl"
#include "Math.hlsl"

struct VSInput
{
    float3 Pos      : ATTRIB0;
    float2 UV0      : ATTRIB1;

	float4 World0	: ATTRIB2;
	float4 World1	: ATTRIB3;
	float4 World2 	: ATTRIB4;
	float4 World3 	: ATTRIB5;
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
    
	WorldPos = transformWorldPos.xyz / transformWorldPos.w;
	ClipPos  = mul(float4(WorldPos, 1.0), mGlobals.mCamera.mViewProj);
    UV 	  = VSIn.UV0;
}
