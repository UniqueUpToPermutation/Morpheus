#include "BasicStructures.hlsl"

cbuffer Globals
{
    RendererGlobalData mGlobals;
};

struct VSInput 
{
	float3 Position : ATTRIB0;
	float2 UV 		: ATTRIB1; 

	float4 World0	: ATTRIB2;
	float4 World1	: ATTRIB3;
	float4 World2 	: ATTRIB4;
	float4 World3 	: ATTRIB5;
};

struct PSInput 
{ 
    float4 Position	: SV_POSITION;
	float2 UV	 	: TEXCOORD;
};

void main(in uint VertId : SV_VertexID,
	in VSInput VSIn,
	out PSInput PSIn) 
{
    float4x4 InstanceMatr = 
		MatrixFromRows(
			VSIn.World0, 
			VSIn.World1, 
			VSIn.World2, 
			VSIn.World3);

	float4 worldPos = mul(float4(VSIn.Position, 1.0), 
		InstanceMatr);

	PSIn.Position  	= mul(worldPos, mGlobals.mCamera.mViewProj);

    PSIn.UV 		= VSIn.UV;
}