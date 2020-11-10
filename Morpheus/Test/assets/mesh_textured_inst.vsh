struct DefaultRendererGlobals {
	float4x4 	mView;
	float4x4 	mProjection;
	float4x4 	mViewProjection;
	float4x4 	mViewProjectionInverse;
	float3 		mEye;
	float 		mTime;
};

cbuffer Globals
{
    DefaultRendererGlobals mGlobals;
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

	PSIn.Position  	= mul(worldPos, 
		mGlobals.mViewProjection);

    PSIn.UV 		= VSIn.UV;
}