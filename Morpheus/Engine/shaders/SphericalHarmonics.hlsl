#ifndef _SPHERICAL_HARMONICS_H_
#define _SPHERICAL_HARMONICS_H_

#define SH_C_0 0.28209479177
#define SH_C_1 0.4886025119
#define SH_C_2n2 1.09254843059
#define SH_C_2n1 SH_C_2n2
#define SH_C_20 0.31539156525
#define SH_C_21 SH_C_2n2
#define SH_C_22 0.54627421529

float4 reconstructSH9(float4 coeffs[9], float3 normal) {
	return (
		SH_C_0 * coeffs[0] +
		SH_C_1 * normal.y * coeffs[1] + 
		SH_C_1 * normal.z * coeffs[2] + 
		SH_C_1 * normal.x * coeffs[3] + 
		SH_C_2n2 * normal.x * normal.y * coeffs[4] +
		SH_C_2n1 * normal.y * normal.z * coeffs[5] +
		SH_C_20 * (-normal.x * normal.x - 
			normal.y * normal.y + 2.0 * normal.z * normal.z) * coeffs[6] +
		SH_C_21 * normal.z * normal.x * coeffs[7] + 
		SH_C_22 * (normal.x * normal.x - normal.y * normal.y) * coeffs[8]
    );
}

float3 reconstructSH9(float3 coeffs[9], float3 normal) {
	return (
		SH_C_0 * coeffs[0] +
		SH_C_1 * normal.y * coeffs[1] + 
		SH_C_1 * normal.z * coeffs[2] + 
		SH_C_1 * normal.x * coeffs[3] + 
		SH_C_2n2 * normal.x * normal.y * coeffs[4] +
		SH_C_2n1 * normal.y * normal.z * coeffs[5] +
		SH_C_20 * (-normal.x * normal.x - 
			normal.y * normal.y + 2.0 * normal.z * normal.z) * coeffs[6] +
		SH_C_21 * normal.z * normal.x * coeffs[7] + 
		SH_C_22 * (normal.x * normal.x - normal.y * normal.y) * coeffs[8]
    );
}

float2 reconstructSH9(float2 coeffs[9], float3 normal) {
	return (
		SH_C_0 * coeffs[0] +
		SH_C_1 * normal.y * coeffs[1] + 
		SH_C_1 * normal.z * coeffs[2] + 
		SH_C_1 * normal.x * coeffs[3] + 
		SH_C_2n2 * normal.x * normal.y * coeffs[4] +
		SH_C_2n1 * normal.y * normal.z * coeffs[5] +
		SH_C_20 * (-normal.x * normal.x - 
			normal.y * normal.y + 2.0 * normal.z * normal.z) * coeffs[6] +
		SH_C_21 * normal.z * normal.x * coeffs[7] + 
		SH_C_22 * (normal.x * normal.x - normal.y * normal.y) * coeffs[8]
    );
}

float reconstructSH9(float coeffs[9], float3 normal) {
	return (
		SH_C_0 * coeffs[0] +
		SH_C_1 * normal.y * coeffs[1] + 
		SH_C_1 * normal.z * coeffs[2] + 
		SH_C_1 * normal.x * coeffs[3] + 
		SH_C_2n2 * normal.x * normal.y * coeffs[4] +
		SH_C_2n1 * normal.y * normal.z * coeffs[5] +
		SH_C_20 * (-normal.x * normal.x - 
			normal.y * normal.y + 2.0 * normal.z * normal.z) * coeffs[6] +
		SH_C_21 * normal.z * normal.x * coeffs[7] + 
		SH_C_22 * (normal.x * normal.x - normal.y * normal.y) * coeffs[8]
    );
}

void SH9(float3 position, out float sh[9]) {
	sh[0] = SH_C_0;
	sh[1] = SH_C_1 * position.y;
	sh[2] = SH_C_1 * position.z;
	sh[3] = SH_C_1 * position.x;
	sh[4] = SH_C_2n2 * position.x * position.y;
	sh[5] = SH_C_2n1 * position.y * position.z;
	sh[6] = SH_C_20 * (-position.x * position.x - 
		position.y * position.y + 2.0 * position.z * position.z);
	sh[7] = SH_C_21 * position.z * position.x;
	sh[8] = SH_C_22 * (position.x * position.x - 
		position.y * position.y);
}

void MakeLambertFilter9(out float filter[9]) {
	// l = 0
	filter[0] = 1.0;

	// l = 1
	for (uint i = 1; i < 4; ++i)
		filter[i] = 2.0 / 3.0;

	// l = 2
	for (uint i = 4; i < 9; ++i)
		filter[i] = 1.0 / 4.0;
}

#endif