#define FLT_EPS 0.0000001

vec3 ToneMapping(vec3 x)
{
    return x / (x + vec3(1.0f)); // Reinhard tonemap
}

vec3 InverseToneMapping(vec3 x)
{
    return x / max((vec3(1.0f) - x), vec3(FLT_EPS));
}
// 色彩空间转换
// https://software.intel.com/en-us/node/503873
vec3 RGBToYCoCg(vec3 c)
{
    // Y = R/4 + G/2 + B/4
    // Co = R/2 - B/2
    // Cg = -R/4 + G/2 - B/4
    return vec3(
        c.x / 4.0 + c.y / 2.0 + c.z / 4.0,
        c.x / 2.0 - c.z / 2.0,
        -c.x / 4.0 + c.y / 2.0 - c.z / 4.0);
}

// https://software.intel.com/en-us/node/503873
vec3 YCoCgToRGB(vec3 c)
{
    // R = Y + Co - Cg
    // G = Y + Cg
    // B = Y - Co - Cg
    // return clamp(vec3(
    //                  c.x + c.y - c.z,
    //                  c.x + c.z,
    //                  c.x - c.y - c.z),
    //              0.0,
    //              1.0);

	return vec3(c.x + c.y - c.z, c.x + c.z, c.x - c.y - c.z);
}

float Saturate(float x)
{
	return clamp(x, 0.0f, 1.0f);
}

vec2 Saturate(vec2 x)
{
	return clamp(x, 0.0f, 1.0f);
}

vec3 Saturate(vec3 x)
{
	return clamp(x, 0.0f, 1.0f);
}

vec4 Saturate(vec4 x)
{
	return clamp(x, 0.0f, 1.0f);
}

float Lerp(float a, float b, float c)
{
    return (1 - c) * a + c * b;
}

vec2 Lerp(vec2 a, vec2 b, float c)
{
    return (1 - c) * a + c * b;
}

vec3 Lerp(vec3 a, vec3 b, float c)
{
    return (1 - c) * a + c * b;
}

vec4 Lerp(vec4 a, vec4 b, float c)
{
    return (1 - c) * a + c * b;
}

float Square( float x )
{
	return x*x;
}

vec2 Square( vec2 x )
{
	return x*x;
}

vec3 Square( vec3 x )
{
	return x*x;
}

vec4 Square( vec4 x )
{
	return x*x;
}

float Pow2( float x )
{
	return x*x;
}

vec2 Pow2( vec2 x )
{
	return x*x;
}

vec3 Pow2( vec3 x )
{
	return x*x;
}

vec4 Pow2( vec4 x )
{
	return x*x;
}

float Pow3( float x )
{
	return x*x*x;
}

vec2 Pow3( vec2 x )
{
	return x*x*x;
}


vec3 Pow3( vec3 x )
{
	return x*x*x;
}

vec4 Pow3( vec4 x )
{
	return x*x*x;
}

float Pow4( float x )
{
	float xx = x*x;
	return xx * xx;
}

vec2 Pow4( vec2 x )
{
	vec2 xx = x*x;
	return xx * xx;
}

vec3 Pow4( vec3 x )
{
	vec3 xx = x*x;
	return xx * xx;
}

vec4 Pow4( vec4 x )
{
	vec4 xx = x*x;
	return xx * xx;
}

float Pow5( float x )
{
	float xx = x*x;
	return xx * xx * x;
}

vec2 Pow5( vec2 x )
{
	vec2 xx = x*x;
	return xx * xx * x;
}

vec3 Pow5( vec3 x )
{
	vec3 xx = x*x;
	return xx * xx * x;
}

vec4 Pow5( vec4 x )
{
	vec4 xx = x*x;
	return xx * xx * x;
}