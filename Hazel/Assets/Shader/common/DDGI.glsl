#ifndef DDGI_GLSL
#define DDGI_GLSL
#include "math.glsl"
uint DDGIGetProbesPerPlane(uvec3 probeCount){
    // 针对Y-up系统
    return probeCount.x  * probeCount.z;
}
uvec3 DDGIGetProbeCoords(uint probeIndex, DDGISetting volume)
{
    uvec3 probeCoords;
    probeCoords.x = probeIndex % volume.probeCount.x;
    probeCoords.y = probeIndex / (volume.probeCount.x * volume.probeCount.z);
    probeCoords.z = (probeIndex / volume.probeCount.x) % volume.probeCount.z;
    return probeCoords;
}

int DDGIGetScrollingProbeIndex(vec3 probeCoords,DDGISetting volume){
    // TODO:探针面对无限场景的优化处理，还没懂
    return 1;
}

vec3 DDGIGetProbeWorldPosition(uvec3 probeCoords, DDGISetting volume){
    vec3 probeGridWorldPosition  = probeCoords * volume.gridStep;   // 网格坐标考虑探针间距后的坐标
    vec3 probeGridShift = (volume.gridStep * (volume.probeCount - 1)) * 0.5; // 偏移
    vec3 probeWorldPosition = probeGridWorldPosition - probeGridShift; // 探针在网格中的位置
    probeWorldPosition += volume.centerPosition; // 加上volume的世界位置
    return probeWorldPosition;
}

/**
 * 通过Spherical fibonacci mapping随机生成一组球面上的方向。
 * Computes a low discrepancy spherically distributed direction on the unit sphere,
 * for the given index in a set of samples. Each direction is unique in
 * the set, but the set of directions is always the same.
 */
vec3 RTXGISphericalFibonacci(float sampleIndex, float numSamples)
{
    const float b = (sqrt(5.f) * 0.5f + 0.5f) - 1.f;
    float phi = TwoPI * fract(sampleIndex * b);
    float cosTheta = 1.f - (2.f * sampleIndex + 1.f) * (1.f / numSamples);
    float sinTheta = sqrt(Saturate(1.f - (cosTheta * cosTheta)));

    return vec3((cos(phi) * sinTheta), (sin(phi) * sinTheta), cosTheta);
}

uvec3 DDGIGetRayDataTexelCoords(uint rayIndex, uint probeIndex, DDGISetting volume)
{
    uint probesPerPlane = DDGIGetProbesPerPlane(volume.probeCount);

    uvec3 coords;
    coords.x = rayIndex;
    coords.z = probeIndex / probesPerPlane;
    coords.y = probeIndex - (coords.z * probesPerPlane);

    return coords;
}
#endif