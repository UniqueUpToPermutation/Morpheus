#ifndef SPRITE_BATCH_STRUCTURES_H_
#define SPRITE_BATCH_STRUCTURES_H_

#ifdef __cplusplus

namespace Morpheus {
	struct SpriteBatchVSInput
	{
		// Use W component as rotation
		float4 mPos;
		float4 mColor;

		float2 mUVTop;
		float2 mUVBottom;

		float2 mSize;
		float2 mOrigin;
	};
}

#else

struct SpriteBatchVSInput
{
	// Use W component as rotation
    float4 mPos     	: ATTRIB0;
	float4 mColor 		: ATTRIB1;

	float2 mUVTop 		: ATTRIB2;
	float2 mUVBottom 	: ATTRIB3;

	float2 mSize 		: ATTRIB4;
	float2 mOrigin		: ATTRIB5;
};

struct SpriteBatchGSInput
{
	float4 mPos  		: SV_Position;
	float4 mColor		: COLOR0;
	float4 mUVX			: TEXCOORD0;
	float4 mUVY			: TEXCOORD1;
	float2 mUVTop		: TEXCOORD2;
	float2 mUVBottom	: TEXCOORD3;
};

struct SpriteBatchPSInput 
{
	float4 mPos 		: SV_Position;
	float4 mColor		: COLOR0;
	float2 mUV			: TEXCOORD0;
};

#endif

#endif