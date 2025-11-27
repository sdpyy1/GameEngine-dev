#version 450 core
#ifdef COMPUTE_SHADER
#define LOCAL_SIZE 8
#include "include/SkyCommon.glslh"
// TODO: 改Set = 1，其他用common
layout(rgba32f, binding = 0) uniform writeonly image2D SkyViewLut;
layout(binding = 1) uniform sampler2D u_TransmittanceLut;
layout(binding = 2) uniform sampler2D u_MultiScatteringLut;
layout(std140, set = 0, binding = 3) uniform SceneData
{
	DirectionalLight DirectionalLights;
	float EnvironmentMapIntensity;
} u_Scene;
layout(set = 0,binding = 4) uniform CameraDataUniform {
    mat4 view;
    mat4 proj;
	mat4 viewProj;
	float width;
	float height;
	float Near;
	float Far;
	vec3 CameraPosition;
} u_CameraData;
layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;
void main()
{
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);

	AtmosphereParameter Atmosphere = BuildAtmosphereParameter();

	ivec2 lutSize = imageSize(SkyViewLut);

	vec2 uv = vec2(texelCoord) / vec2(lutSize);
    vec3 viewDir = UVToViewDir(uv);
	vec3 lightDir = normalize(-u_Scene.DirectionalLights.Direction);
	float h = u_CameraData.CameraPosition.y - Atmosphere.SeaLevel + Atmosphere.PlanetRadius;
	vec3 eyePos = vec3(0, h, 0);

	vec3 color = GetSkyView(Atmosphere, eyePos, viewDir, lightDir, -1.0f,u_TransmittanceLut, u_MultiScatteringLut);

    imageStore(SkyViewLut, texelCoord, vec4(color, 1.0));

}

#endif


