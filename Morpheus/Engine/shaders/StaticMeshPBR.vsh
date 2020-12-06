#include "PBRStructures.hlsl"
#include "VertexProcessing.hlsl"
#include "BasicStrctures.hlsl"

struct VSInput
{
    float3 Pos      : ATTRIB0;
    float3 Normal   : ATTRIB1;
    float2 UV0      : ATTRIB2;

	float4 World0	: ATTRIB3;
	float4 World1	: ATTRIB4;
	float4 World2 	: ATTRIB5;
	float4 World3 	: ATTRIB6;
};

cbuffer cbGlobalAttribs
{
    RendererGlobalData mGlobals;
}
    
void main(in  VSInput  VSIn,
          out float4 ClipPos  : SV_Position,
          out float3 WorldPos : WORLD_POS,
          out float3 Normal   : NORMAL,
          out float2 UV0      : UV0) 
{
    float4x4 Transform = MatrixFromRows(
		VSIn.World0, 
		VSIn.World1, 
		VSIn.World2, 
		VSIn.World3);

    GLTF_TransformedVertex TransformedVert = GLTF_TransformVertex(VSIn.Pos, VSIn.Normal, Transform);

    ClipPos  = mul(float4(TransformedVert.WorldPos, 1.0), mGlobals.mCamera.mViewProj);
    WorldPos = TransformedVert.WorldPos;
    Normal   = TransformedVert.Normal;
    UV0      = VSIn.UV0;
}
