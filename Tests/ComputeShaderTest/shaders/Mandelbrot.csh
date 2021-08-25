
struct UniformGlobals {
	float mTime;
	float mPadding0;
	float mPadding1;
	float mPadding2;
};

#ifndef __cplusplus

cbuffer mUniformGlobals {
	UniformGlobals mGlobals;
}

RWTexture2D<float4> mOutput;

float distanceToMandelbrot(float2 c)
{
    {
        float c2 = dot(c, c);
        // skip computation inside M1 - http://iquilezles.org/www/articles/mset_1bulb/mset1bulb.htm
        if (256.0*c2*c2 - 96.0*c2 + 32.0*c.x - 3.0 < 0.0) return 0.0;
        // skip computation inside M2 - http://iquilezles.org/www/articles/mset_2bulb/mset2bulb.htm
        if (16.0*(c2+2.0*c.x+1.0) - 1.0 < 0.0) return 0.0;
    }

    // iterate
    float di =  1.0;
    float2 z  = float2(0.0);
    float m2 = 0.0;
    float2 dz = float2(0.0);
    for (int i = 0; i < 300; i++)
    {
        if (m2 > 1024.0) {
			di=0.0; 
			break; 
		}

		// Z' -> 2·Z·Z' + 1
        dz = 2.0 * float2(z.x*dz.x-z.y*dz.y, z.x*dz.y + z.y*dz.x) + float2(1.0, 0.0);
			
        // Z -> Z² + c			
        z = float2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
			
        m2 = dot(z,z);
    }

    // distance	
	// d(c) = |Z|·log|Z|/|Z'|
	float d = 0.5 * sqrt(dot(z, z) / dot(dz, dz)) * log(dot(z, z));
    if (di > 0.5) 
		d = 0.0;
	
    return d;
}

[numthreads(CELL_SIZE_X, CELL_SIZE_Y, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width, height;

	mOutput.GetDimensions(width, height);

	float2 uv = float2((dispatchThreadId.x + 0.5) / (float)width,
		(dispatchThreadId.y + 0.5) / (float)height);

	float2 p = 2.0 * (uv - float2(0.5));

	float tz = float(0.5 - 0.5 * cos(0.225 * mGlobals.mTime));
    float zoo = pow( 0.5, 13.0 * tz);
	float2 c = float2(-0.05, .6805) + p * zoo;

    float d = distanceToMandelbrot(c);
    // do some soft coloring based on distance
	d = clamp(pow(4.0 * d / zoo, 0.2), 0.0, 1.0);
    
    float3 col = float3(d);

	mOutput[dispatchThreadId.xy] = float4(col, 1.0);
}

#endif