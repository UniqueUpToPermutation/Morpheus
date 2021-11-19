#include "TexturedStaticMesh.hlsl"

struct VSInput
{
    float3 Pos      : ATTRIB0;
    float2 UV0      : ATTRIB1;
};

cbuffer Globals {
    ViewAttribs mViewData;
}

cbuffer Instance {
    InstanceData mInstanceData;
}

void main(in  VSInput VSIn,
    out float4 ClipPos  	: SV_Position,
	out float3 WorldPos 	: WORLD_POS,
	out float2 UV	      	: UV0) 
{
	float4 transformWorldPos = mul(mInstanceData.mWorld, float4(VSIn.Pos, 1.0));
	WorldPos = transformWorldPos.xyz / transformWorldPos.w;
	ClipPos  = mul(float4(WorldPos, 1.0), mViewData.mCamera.mViewProj);
    UV 	  = VSIn.UV0;
}
