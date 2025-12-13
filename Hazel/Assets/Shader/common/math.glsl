#ifndef MATH_GLSL
#define MATH_GLSL



#define FLT_EPS 0.0000001

float saturate(float x)
{
	return clamp(x, 0.0f, 1.0f);
}

vec2 saturate(vec2 x)
{
	return clamp(x, 0.0f, 1.0f);
}

vec3 saturate(vec3 x)
{
	return clamp(x, 0.0f, 1.0f);
}

vec4 saturate(vec4 x)
{
	return clamp(x, 0.0f, 1.0f);
}

float lerp(float a, float b, float c)
{
    return (1 - c) * a + c * b;
}

vec2 lerp(vec2 a, vec2 b, float c)
{
    return (1 - c) * a + c * b;
}

vec3 lerp(vec3 a, vec3 b, float c)
{
    return (1 - c) * a + c * b;
}

vec4 lerp(vec4 a, vec4 b, float c)
{
    return (1 - c) * a + c * b;
}

float square( float x )
{
	return x*x;
}

vec2 square( vec2 x )
{
	return x*x;
}

vec3 square( vec3 x )
{
	return x*x;
}

vec4 square( vec4 x )
{
	return x*x;
}

float pow2( float x )
{
	return x*x;
}

vec2 pow2( vec2 x )
{
	return x*x;
}

vec3 pow2( vec3 x )
{
	return x*x;
}

vec4 pow2( vec4 x )
{
	return x*x;
}

float pow3( float x )
{
	return x*x*x;
}

vec2 pow3( vec2 x )
{
	return x*x*x;
}


vec3 pow3( vec3 x )
{
	return x*x*x;
}

vec4 pow3( vec4 x )
{
	return x*x*x;
}

float pow4( float x )
{
	float xx = x*x;
	return xx * xx;
}

vec2 pow4( vec2 x )
{
	vec2 xx = x*x;
	return xx * xx;
}

vec3 pow4( vec3 x )
{
	vec3 xx = x*x;
	return xx * xx;
}

vec4 pow4( vec4 x )
{
	vec4 xx = x*x;
	return xx * xx;
}

float pow5( float x )
{
	float xx = x*x;
	return xx * xx * x;
}

vec2 pow5( vec2 x )
{
	vec2 xx = x*x;
	return xx * xx * x;
}

vec3 pow5( vec3 x )
{
	vec3 xx = x*x;
	return xx * xx * x;
}

vec4 pow5( vec4 x )
{
	vec4 xx = x*x;
	return xx * xx * x;
}
float rcp(float x) { return 1 / x;}

float rsqrt(float x) { return 1 / sqrt(x);}

// TODO: 换个位置存放
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
#endif