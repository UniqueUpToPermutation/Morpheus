#include "Utils/SphericalHarmonics.hlsl"
#include "Utils/Sampling.hlsl"

#ifndef SAMPLE_COUNT
#	define SAMPLE_COUNT 5000
#endif

TextureCube mEnvironmentMap;
SamplerState mEnvironmentMap_sampler;

RWBuffer<float4> mCoeffsOut;

[numthreads(1, 1, 1)]
void main(uint3 Idx : SV_DISPATCHTHREADID)
{
	float4 colorResults[9];
	float lambertFilter[9];

	MakeLambertFilter9(lambertFilter);

	for (uint i = 0; i < 9; ++i) {
		colorResults[i] = float4(0.0, 0.0, 0.0, 0.0);
	}

    for (uint i = 0; i < SAMPLE_COUNT; ++i) {
		float2 xi = Hammersley2D(i, SAMPLE_COUNT);
		float3 dir = SampleSphere(xi);
		
		float4 colorSample = mEnvironmentMap.SampleLevel(mEnvironmentMap_sampler, dir, 0);

		float shModes[9];
		SH9(dir, shModes);

		// Integrate with Monte Carlo
		for (uint i = 0; i < 9; ++i) {
			colorResults[i] += colorSample * shModes[i];
		}
	}

	// Compensate for the sample count
	for (uint i = 0; i < 9; ++i) {
		colorResults[i] *= 4.0 * PI / SAMPLE_COUNT;
	}

	// Apply lambert filter
	for (uint i = 0; i < 9; ++i) {
		mCoeffsOut[i] = colorResults[i] * lambertFilter[i];
	}
}
