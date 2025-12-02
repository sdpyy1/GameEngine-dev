#version 450 core
#ifdef COMPUTE_SHADER
#define LOCAL_SIZE 8
#include "common/Sky.glsl"
#include "common/common.glsl"

layout(set = 1,binding = 0) uniform sampler u_Sampler[];

layout(set = 2, rgba32f, binding = 0) uniform writeonly image2D SkyViewLut;
layout(set = 2, binding = 1) uniform texture2D u_TransmittanceLut;
layout(set = 2, binding = 2) uniform texture2D u_MultiScatteringLut;



layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;
void main()
{
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);

	AtmosphereParameter Atmosphere = BuildAtmosphereParameter();

	ivec2 lutSize = imageSize(SkyViewLut);

	vec2 uv = vec2(texelCoord) / vec2(lutSize);
    vec3 viewDir = UVToViewDir(uv);
	vec3 lightDir = normalize(-u_LightInfo.data.dirLights.direction);
	float h = u_CameraData.data.CameraPosition.y - Atmosphere.SeaLevel + Atmosphere.PlanetRadius;
	vec3 eyePos = vec3(0, h, 0);

	vec3 color = GetSkyView(Atmosphere, eyePos, viewDir, lightDir, -1.0f, u_TransmittanceLut, u_MultiScatteringLut,u_Sampler[1]);

    imageStore(SkyViewLut, texelCoord, vec4(color, 1.0));

}

#endif


