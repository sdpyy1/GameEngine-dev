#version 450 core
#include "include/SkyCommon.glslh"
#include "common/Common.glsl"

#ifdef VERTEX_SHADER
vec2 NDC[3] = vec2[](
    vec2(-1.0, -1.0), 
    vec2( 3.0, -1.0), 
    vec2(-1.0,  3.0) 
);
layout(location = 0) out vec2 out_texCoord;
layout(location = 1) out vec3 worldPosition;
void main(){
	vec4 position = vec4(NDC[gl_VertexIndex], 1.0, 1.0);
	gl_Position = position;
    out_texCoord = (NDC[gl_VertexIndex] + 1.0) * 0.5;
	worldPosition = (u_CameraData.data.InverseViewProj * position).xyz;
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) in vec2 in_texCoord;
layout(location = 1) in vec3 worldPosition;
layout(location = 0) out vec4 out_color;
layout(set = 2,binding = 0) uniform textureCube SkyTexture;
layout(set = 2,binding = 1) uniform texture2D u_SkyViewLut;
layout(set = 2,binding = 2) uniform texture2D u_TransmittanceLut;
layout(set = 2,binding = 3) uniform texture2D u_MultiScatteringLut;
layout(set = 1, binding = 0) uniform sampler u_Sampler[];


vec3 GetSunDisk(in AtmosphereParameter param, vec3 eyePos, vec3 viewDir, vec3 lightDir)
{
    // 计算入射光照
    float cosine_theta = dot(viewDir, lightDir);
    float theta = acos(cosine_theta) * (180.0 / PI);
    vec3 sunLuminance = param.SunLightColor * param.SunLightIntensity;

    // 判断光线是否被星球阻挡
    float disToPlanet = RayIntersectSphere(vec3(0,0,0), param.PlanetRadius, eyePos, viewDir);
    if(disToPlanet >= 0) return vec3(0,0,0);

    // 和大气层求交
    float disToAtmosphere = RayIntersectSphere(vec3(0,0,0), param.PlanetRadius + param.AtmosphereHeight, eyePos, viewDir);
    if(disToAtmosphere < 0) return vec3(0,0,0);

    // 计算衰减
    sunLuminance *= TransmittanceToAtmosphere(param, eyePos, viewDir, u_TransmittanceLut,u_Sampler[0]);

    if(theta < param.SunDiskAngle) return sunLuminance;
    return vec3(0,0,0);
}
void main(){
	uint isDynamicSky = 0;
	vec4 color = vec4(0,0,0,1);
	vec3 v_dir = normalize(worldPosition);
	if(isDynamicSky == 0){
		out_color = texture(samplerCube(SkyTexture,u_Sampler[0]),v_dir);
	}else{
		AtmosphereParameter Atmosphere = BuildAtmosphereParameter();
		vec3 lightDir = normalize(-u_LightInfo.data.dirLights.direction);
		float h = u_CameraData.data.CameraPosition.y - Atmosphere.SeaLevel + Atmosphere.PlanetRadius;
		vec3 eyePos = vec3(0, h, 0);
		color.rgb += texture(sampler2D(u_SkyViewLut,u_Sampler[0]), ViewDirToUV(v_dir)).rgb;
		color.rgb += GetSunDisk(Atmosphere, eyePos, v_dir, lightDir);
		out_color = color;

	}
}
#endif
