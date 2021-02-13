#include "SpriteBatchStructures.hlsl"
#include "BasicStructures.hlsl"

cbuffer Globals 
{
	RendererGlobalData mGlobals;
};

void main(in SpriteBatchVSInput VSIn,
    out SpriteBatchGSInput GSIn) 
{
    float4 pos = float4(VSIn.mPos.xyz, 1.0);
	float rotation = VSIn.mPos.w;

	float2 uvSlice = VSIn.mUVBottom - VSIn.mUVTop;

	float cosRot = cos(rotation);
	float sinRot = sin(rotation);

	float2x2 rotMat = float2x2(
		cosRot, sinRot, 
		-sinRot, cosRot);

	float2 halfSize = VSIn.mSize / 2;

	float2 uvx = float2(halfSize.x, 0.0);
	float2 uvy = float2(0.0, halfSize.y);

	uvx = mul(rotMat, uvx);
	uvy = mul(rotMat, uvy);

	float2 toCenter = halfSize - VSIn.mOrigin;

	toCenter = mul(rotMat, toCenter);

	pos.xy += toCenter;

	GSIn.mPos = mul(pos, mGlobals.mCamera.mViewProj);
	GSIn.mUVX = mul(float4(uvx, 0.0, 0.0), mGlobals.mCamera.mViewProj);
	GSIn.mUVY = mul(float4(uvy, 0.0, 0.0), mGlobals.mCamera.mViewProj);

	GSIn.mUVTop = VSIn.mUVTop;
	GSIn.mUVBottom = VSIn.mUVBottom;
	GSIn.mColor = VSIn.mColor;
}