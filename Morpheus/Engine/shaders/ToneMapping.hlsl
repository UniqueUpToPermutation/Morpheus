#include "ToneMappingStructures.hlsl"

#ifndef TONE_MAPPING_MODE
#define TONE_MAPPING_MODE TONE_MAPPING_MODE_REINHARD
#endif

float3 ToneMap(float3 inColor, ToneMappingAttribs attribs) {
#if TONE_MAPPING_MODE == TONE_MAPPING_MODE_REINHARD
	float3 color = inColor * attribs.mExposure;

	// Reinhard tonemapping operator.
	// see: "Photographic Tone Reproduction for Digital Images", eq. 4
	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance/(attribs.mPureWhite * attribs.mPureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
	float3 mappedColor = (mappedLuminance / luminance) * color;
	return mappedColor;
#else
	return rgb;
#endif
}