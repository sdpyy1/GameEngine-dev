#version 450 core
#ifdef COMPUTE_SHADER
#define LOCAL_SIZE 8
#include "../common/Sky.glsl"

layout(rgba32f, binding = 0) uniform writeonly image2D transmittanceLut;

layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;
void main()
{
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);

	AtmosphereParameter Atmosphere = BuildAtmosphereParameter();


	float bottomRadius = Atmosphere.PlanetRadius;
	float topRadius = Atmosphere.PlanetRadius + Atmosphere.AtmosphereHeight;


    ivec2 lutSize = imageSize(transmittanceLut);
    float cos_theta;
    vec2 uv = vec2(texelCoord) / vec2(lutSize);
	float r = 0.0;

	// u����cos��1��-1��v���򣺸߶ȴ�0��1
    UvToTransmittanceLutParams(bottomRadius, topRadius, uv, cos_theta, r);
	float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
	vec3 CameraPosition = vec3(0.0, r, 0.0);
	vec3 viewDir = vec3(sin_theta, cos_theta, 0);

	float dis = RayIntersectSphere(vec3(0,0,0), topRadius, CameraPosition, viewDir);
	vec3 hitPoint = CameraPosition + viewDir * dis;
	vec3 color = Transmittance(Atmosphere, CameraPosition, hitPoint);
    imageStore(transmittanceLut, texelCoord, vec4(color, 1.0));
}

#endif


