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

	vPos = mGlobals.mViewProjectionInverse * vPos;
	vPos /= vPos.w;

	PSIn.Direction = vPos.xyz - mGlobals.mEye;
}