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
#include "rand.glsl"

vec3 GetRandomCosineDirectionOnHemisphere(vec3 direction,inout Rand seed)
{
    // Choose random points on the unit sphere offset along the surface normal
    // to produce a cosine distribution of random directions.
    float a = RandFloat(seed) * TwoPI;
    float z = RandFloat(seed) * 2.f - 1.f;
    float r = sqrt(1.f - z * z);

    vec3 p = vec3(r * cos(a), r * sin(a), z) + direction;
    return normalize(p);
}

float LinearDepthToNonLinear(float linearDepth, Camera camera) {
    return (linearDepth - camera.Near) / (camera.Far - camera.Near);
}

float NonLinearDepthToLinear(float nonLinearDepth, Camera camera) {
    return nonLinearDepth * (camera.Far - camera.Near) + camera.Near;
}

/*
	传入自定义深度（线性深度[near-far]）
*/
vec3 SceenToWorldCustomDepth(vec2 uv, float viewZ, Camera camera)
{
    vec2 ndcXY = uv * 2.0 - 1.0;
    vec4 ndcPos = vec4(ndcXY, 0.0, 1.0);
    vec4 viewPos = camera.invProj * ndcPos;
    viewPos /= viewPos.w;
    vec3 viewDir = normalize(viewPos.xyz);
    float t = viewZ / (-viewDir.z);
    vec3 finalViewPos = viewDir * t;
    vec4 worldPos = camera.invView * vec4(finalViewPos, 1.0);
    return worldPos.xyz;
}

vec3 SceenToWorld(vec2 uv, float viewZ, Camera camera)
{
    vec2 ndcXY = uv * 2.0 - 1.0;
    vec4 ndcPos = vec4(ndcXY, viewZ, 1.0);
    vec4 viewPos = camera.InverseViewProj * ndcPos;
    viewPos /= viewPos.w;
    return viewPos.xyz;
}

vec3 SceenToView(vec2 uv, float depth, Camera camera){
	vec3 ndc = vec3(uv * 2.0 - 1.0, depth);
    mat4 VInv = camera.invProj;
	vec4 world = VInv * vec4(ndc, 1.0);
	return world.xyz / world.w;
}

#endif