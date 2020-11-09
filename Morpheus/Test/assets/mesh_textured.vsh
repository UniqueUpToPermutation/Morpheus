struct DefaultRendererGlobals {
	float4x4 mWorld;
	float4x4 mView;
	float4x4 mProjection;
	float4x4 mWorldViewProjection;
	float4x4 mWorldView;
	float4x4 mWorldViewProjectionInverse;
	float4x4 mViewProjectionInverse;
	float4x4 mWorldInverseTranspose;
	float4x4 mWorldViewInverseTranspose;
	float3 mEye;
	float mTime;
};

cbuffer Globals
{
    DefaultRendererGlobals mGlobals;
};

struct VSInput 
{
	float3 Position : ATTRIB0;
	float2 UV 		: ATTRIB1; 	
};

struct PSInput 
{ 
    float4 Position   : SV_POSITION;
	float2 UV	 : TEXCOORD;
};

void main(in uint VertId : SV_VertexID,
		in  VSInput VSIn,
        out PSInput PSIn) 
{
	PSIn.Position   = mul(float4(VSIn.Position, 1.0), mGlobals.mWorldViewProjection);

    PSIn.UV 		= VSIn.UV;
}