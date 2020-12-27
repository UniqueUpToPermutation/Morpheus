#ifndef _GAMMA_H_
#define _GAMMA_H_

#ifndef PBR_MANUAL_SRGB
#   define PBR_MANUAL_SRGB 1
#endif

#ifndef SRGB_FAST_APPROXIMATION
#   define SRGB_FAST_APPROXIMATION 1
#endif

float3 SRGBToLinear(float3 srgbIn)
{
#ifdef PBR_MANUAL_SRGB
#   ifdef SRGB_FAST_APPROXIMATION
    	float3 linOut = pow(saturate(srgbIn.xyz), float3(2.2, 2.2, 2.2));
#   else
	    float3 bLess  = step(float3(0.04045, 0.04045, 0.04045), srgbIn.xyz);
	    float3 linOut = mix(srgbIn.xyz/12.92, pow(saturate((srgbIn.xyz + float3(0.055, 0.055, 0.055)) / 1.055), float3(2.4, 2.4, 2.4)), bLess);
#   endif
	    return linOut;
#else
	return srgbIn;
#endif
}

float3 SRGBToLinearNosaturate(float3 srgbIn)
{
#ifdef PBR_MANUAL_SRGB
#   ifdef SRGB_FAST_APPROXIMATION
    	float3 linOut = pow(srgbIn.xyz, float3(2.2, 2.2, 2.2));
#   else
	    float3 bLess  = step(float3(0.04045, 0.04045, 0.04045), srgbIn.xyz);
	    float3 linOut = mix(srgbIn.xyz/12.92, pow((srgbIn.xyz + float3(0.055, 0.055, 0.055)) / 1.055, float3(2.4, 2.4, 2.4)), bLess);
#   endif
	    return linOut;
#else
	return srgbIn;
#endif
}

float4 SRGBToLinear(float4 srgbIn)
{
    return float4(SRGBToLinear(srgbIn.xyz), srgbIn.w);
}

float4 SRGBToLinearNosaturate(float4 srgbIn) 
{
	return float4(SRGBToLinearNosaturate(srgbIn.xyz), srgbIn.w);
}

float3 LinearToSRGB(float3 rgb, float gamma) {
	return pow(rgb, 1.0/gamma);
}

float4 LinearToSRGB(float4 rgba, float gamma) {
	return float4(LinearToSRGB(rgba.rgb, gamma), rgba.a);
}

#endif