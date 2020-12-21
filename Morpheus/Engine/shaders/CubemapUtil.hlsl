#ifndef CUBEMAP_UTIL_H_
#define CUBEMAP_UTIL_H_

float CubemapJacobian(float3 dir) {
	float3 absDir = abs(dir);
	float maxDir = max(max(absDir.x, absDir.y), absDir.z);
	float3 cubeProj = dir / maxDir;
	float mag2 = dot(cubeProj, cubeProj);
	float mag = sqrt(mag2);
	return 1.0 / (mag2 * mag);
}

float CubemapJacobian(float2 surfacePosition) {
	float mag2 = dot(surfacePosition, surfacePosition) + 1.0;
	float mag = sqrt(mag2);
	return 1.0 / (mag2 * mag);
}

#endif