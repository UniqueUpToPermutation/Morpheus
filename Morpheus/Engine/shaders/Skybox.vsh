#include "BasicStructures.hlsl"

cbuffer Globals
{
   	RendererGlobalData mGlobals;
};

struct PSInput 
{ 
    float4 Position   	: SV_POSITION;
	float3 Direction 	: TEXCOORD;
};

void main(in uint VertId : SV_VertexID,
        out PSInput PSIn) 
{
	float4 Pos[4];
    
    Pos[0] = float4(1.0, 1.0, 1.0, 1.0);
	Pos[1] = float4(-1.0, 1.0, 1.0, 1.0);
	Pos[2] = float4(1.0, -1.0, 1.0, 1.0);
	Pos[3] = float4(-1.0, -1.0, 1.0, 1.0);

	float4 vPos = Pos[VertId];

	PSIn.Position = vPos;

	vPos = mul(vPos, mGlobals.mCamera.mViewProjInv);
	vPos /= vPos.w;

	PSIn.Direction = vPos.xyz - mGlobals.mCamera.f4Position.xyz;
}