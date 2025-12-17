#ifndef INTERSECTION_GLSL
#define INTERSECTION_GLSL
#include "struct.glsl"
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
bool FrustumIntersectBox(Frustum frustum, BoundingBox box)
{
    vec3 center = (box.maxBound + box.minBound) * 0.5;
    vec3 extent = (box.maxBound - box.minBound) * 0.5;

    for (int i = 0; i < 6; i++)
    {
        vec4 plane = frustum.planes[i];

        vec3 absN = abs(plane.xyz);
        float radius = dot(absN, extent);

        float distance = dot(plane.xyz, center) + plane.w;

        if (distance < -radius)
            return false;   // 被该平面完全剔除
    }

    return true;
}

#endif