#ifndef _VERTEX_PROCESSING_H_
#define _VERTEX_PROCESSING_H_

#ifndef PI
#   define  PI 3.141592653589793
#endif

float3x3 InverseTranspose3x3(float3x3 M)
{
    // Note that in HLSL, M_t[0] is the first row, while in GLSL, it is the 
    // first column. Luckily, determinant and inverse matrix can be equally 
    // defined through both rows and columns.
    float det = dot(cross(M[0], M[1]), M[2]);
    float3x3 adjugate = float3x3(cross(M[1], M[2]),
		cross(M[2], M[0]),
		cross(M[0], M[1]));
	return adjugate / det;
}

float3 FresnelSchlick(float3 F0, float cosTheta)
{
	return F0 + (float3(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 PerturbNormal(float3 Normal, float3 Tangent, float3 NormalPerturb) 
{
	float3 n = normalize(Normal);
	float3 t = normalize(Tangent);
	float3 b = cross(n, t);

	float3 n_map = NormalPerturb * 2.0 - 1.0;
	return normalize(n_map.x * t - n_map.y * b + n_map.z * n);
}

#endif