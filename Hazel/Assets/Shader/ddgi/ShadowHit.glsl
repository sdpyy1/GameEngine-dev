#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "../common/common.glsl"
#include "../common/DDGI.glsl"
#include "../common/gizmo.glsl"
struct ShadowPayLoad{
	float hitT;
	uint _padding[3];
};
#ifdef RAYCLOSEST_HIT_SHADER
layout(location = 1) rayPayloadInEXT ShadowPayLoad shadowPayload;

void main()
{
	
}
#endif
