#version 450 core
#ifdef COMPUTE_SHADER
#define LOCAL_SIZE 8
#include "../common/Sky.glsl"

layout(rgba32f, binding = 0) uniform writeonly image2D MultiScatteringLut;
layout(binding = 1) uniform texture2D u_TransmittanceLut;
layout(set = 1, binding = 0) uniform sampler u_Sampler[];


layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;
void main()
{
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
	ivec2 lutSize = imageSize(MultiScatteringLut);

    vec2 uv = vec2(texelCoord) / vec2(lutSize);

	AtmosphereParameter Atmosphere = BuildAtmosphereParameter();
	                
	float mu_s = uv.x * 2.0 - 1.0;
	float r = uv.y * Atmosphere.AtmosphereHeight + Atmosphere.PlanetRadius;
	float cos_theta = mu_s;
	float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
	vec3 lightDir = vec3(sin_theta, cos_theta, 0);
    vec3 p = vec3(0, r, 0);
	vec3 color = IntegralMultiScattering(Atmosphere, p, lightDir, u_TransmittanceLut,u_Sampler[0]);

	
    imageStore(MultiScatteringLut, texelCoord, vec4(color, 1.0));
}

#endif


