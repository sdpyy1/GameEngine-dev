#ifndef INTERSECTION_GLSL
#define INTERSECTION_GLSL
#include "struct.glsl"

/*
	三个点组成一个平面方程，法线方向与innerPoint的方向一致
*/
vec4 calculatePlane(vec3 a, vec3 b, vec3 c, vec3 innerPoint) {
    vec3 ab = b - a;
    vec3 ac = c - a;
    vec3 normal = normalize(cross(ab, ac));
    float d = -dot(normal, a);
    float distance = dot(normal, innerPoint) + d;
    if (distance < 0.0) {
        normal = -normal;
        d = -d;
    }

    return vec4(normal, d);
}



BoundingBox BoundingBoxTransform(BoundingBox box, mat4 transform)
{
	vec3 offsets[8] = {	vec3(-1.0f, -1.0f, 1.0f),
						vec3(1.0f, -1.0f, 1.0f),
						vec3(-1.0f, 1.0f, 1.0f),
						vec3(1.0f, 1.0f, 1.0f),
						vec3(-1.0f, -1.0f, -1.0f),
						vec3(1.0f, -1.0f, -1.0f),
						vec3(-1.0f, 1.0f, -1.0f),
						vec3(1.0f, 1.0f, -1.0f)};

	vec3 center = (box.maxBound + box.minBound) * 0.5f;
	vec3 extent = (box.maxBound - box.minBound) * 0.5f;

	vec3 newMaxBound = vec3(-1e30);
	vec3 newMinBound = vec3(1e30);
	for(int i = 0; i < 8; i++)
	{
		vec4 cornerBefore = vec4(extent * offsets[i] + center, 1.0f);
		vec4 corner = transform * cornerBefore;
		corner /= corner.w;

		newMaxBound = vec3(max(newMaxBound.x, corner.x), max(newMaxBound.y, corner.y), max(newMaxBound.z, corner.z));
		newMinBound = vec3(min(newMinBound.x, corner.x), min(newMinBound.y, corner.y), min(newMinBound.z, corner.z));
	}

	BoundingBox newBox;
	newBox.maxBound = newMaxBound;
	newBox.minBound = newMinBound;

	return newBox;
}

float signedDistanceToPlane(vec3 point, vec4 plane) {
    return dot(plane.xyz, point) + plane.w;
}
/*
	当前Frustum的6个平面的法向量都指向内部
	这里的相交判断利用的是SAT（分离轴定理），平面的法向量方向作为分离轴，整个平面在分离轴上就只是一个点，判断AABB盒在分离轴上的范围与这个点的关系来进行相交判断
*/
bool FrustumIntersectBox(Frustum frustum, BoundingBox box)
{
    vec3 center = (box.maxBound + box.minBound) * 0.5;
    vec3 extent = (box.maxBound - box.minBound) * 0.5;

    for (int i = 0; i < 6; i++)
    {
        vec4 plane = frustum.planes[i];

		// 这段蜜汁代码需要结合记录的博客来看，真的不理解清楚，这代码写的简直是顶级防御性编程
		vec3 absN = abs(plane.xyz);
        float radius = dot(absN, extent); // AABB中心到AABB的所有投影位置的最大距离
        float distance = dot(plane.xyz, center) + plane.w;
        if (distance < -radius)
            return false;
    }

    return true;
}
/*
	注意法线必须指向视锥内部，才能使用
*/
bool FrustumIntersectSphere(Frustum frustum, BoundingSphere sphere)
{
    for (int i = 0; i < 6; i++) {
        float distance = signedDistanceToPlane(sphere.center, frustum.planes[i]);
        if (distance < -sphere.radius) {
            return false;
        }
    }
    return true;
}

bool SphereIntersectBox(BoundingSphere sphere, BoundingBox box)
{
    vec3 closestPoint = clamp(
        sphere.center,
        box.minBound,
        box.maxBound
    );
    vec3 delta = closestPoint - sphere.center;
    float distSq = dot(delta, delta);
    return distSq <= sphere.radius * sphere.radius;
}



#endif