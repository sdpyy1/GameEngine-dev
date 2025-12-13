#version 450 core
#include "../common/common.glsl"
#include "../common/Gbuffer.glsl"
#include "../common/DDGI.glsl"

#ifdef VERTEX_SHADER
vec3 kNdcPoints[3] = vec3[]( 
    vec3(-1.0, -1.0, 0.0), 
    vec3( 3.0, -1.0, 0.0), 
    vec3(-1.0,  3.0, 0.0) 
);
layout(location = 0) out vec2 TexCoord;
void main()
{
    gl_Position = vec4(kNdcPoints[gl_VertexIndex].xyz, 1.0);
    TexCoord = (kNdcPoints[gl_VertexIndex].xy + 1.0) * 0.5;
}
#endif

#ifdef FRAGMENT_SHADER
#include "../common/light.glsl" 
#include "../common/shadow.glsl"

layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec4 o_Color;
layout(set = 1, binding = 0) uniform texture2DArray u_DirShadowMapTexture;
layout(set = 1, binding = 1) uniform textureCube u_EnvRadianceTex;
layout(set = 1, binding = 2) uniform textureCube u_EnvIrradianceTex;
layout(set = 1, binding = 3) uniform texture2D u_BRDFLUTTexture;
layout(set = 1, binding = 4) uniform textureCube u_PointShadowMapTexture;
layout(set = 1, binding = 5) uniform texture2DArray ddgi_Irrandiance;
layout(set = 1, binding = 6) uniform texture2DArray ddgi_Distance;
void main()
{
    vec3 WorldPosition = GetGBufferPosition(TexCoord);
	if (WorldPosition == vec3(0.0)) {
		o_Color = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	Camera CAMERAINFO = GetCamera();
	vec3 albedo = GetGBufferAlbedo(TexCoord);
	float roughness =  GetGBufferRoughness(TexCoord);
	float metallic = GetGBufferMetalness(TexCoord);
	vec3 N = GetGBufferNormal(TexCoord);
	vec3 V = normalize(CAMERAINFO.position - WorldPosition);


	
///////////////////////////////////////////// 直接光照 /////////////////////////////////////////////
	vec2 shadowRes = DirectionShadow(u_DirShadowMapTexture,WorldPosition,N);
	vec3 directionLightContribution = CalculateDirectionalLight(albedo, roughness, metallic, N, V) *shadowRes.x ;
	vec3 pointLightContribution = vec3(0);
	for(int i = 0; i < GetPointLightCount(); i++){
		pointLightContribution += CalculatePointLight(albedo, roughness, metallic,WorldPosition, N, V, i) * PointShadow(u_PointShadowMapTexture,WorldPosition,i); 
	}
	vec3 spotLightContribution = vec3(0);
	for(int i = 0; i < GetSpotLightCount(); i++){
		spotLightContribution += CalculateSpotLight(albedo, roughness, metallic,WorldPosition, N, V, i) * SpotShadow(u_PointShadowMapTexture,WorldPosition,i); 
	}
	vec3 lightContribution = directionLightContribution + pointLightContribution + spotLightContribution;
	
///////////////////////////////////////////// IBL /////////////////////////////////////////////
	vec3 iblContribution = CalculateIBLLight(N, V, albedo, metallic, roughness, u_EnvIrradianceTex, u_EnvRadianceTex, u_BRDFLUTTexture) * GetSkySetting().IbLScale; ;
///////////////////////////////////////////// DDGI /////////////////////////////////////////////
	DDGISetting volume = GetDDGISetting();
	vec3 DDGIContribution = vec3(0);
	float blendWeight = DDGIGetVolumeBlendWeight(WorldPosition, volume);
	if(blendWeight > 0){
		DDGIContribution = DDGIGetIrrandianceByWorldPosition(WorldPosition,N,volume,ddgi_Irrandiance,ddgi_Distance);
		DDGIContribution*= blendWeight;
	}
    DDGIContribution = (albedo / PI) * DDGIContribution;


	if(GetRenderSetting().debugDDGI == 1){
		o_Color = vec4(DDGIContribution,1);
		return;
	}else if(GetRenderSetting().debugDDGI == 2){  // TODO:没有DDGI的情况，这些设置需要统一规划
		o_Color = vec4(lightContribution + iblContribution,1);
		return;
	}
	vec3 finalColor = lightContribution + iblContribution + DDGIContribution;

	o_Color = vec4(finalColor,1);






	// DebugCSM
	if(GetShadowSetting().DebugCSM == 1)
	{
		vec3 cascadeColor;
			switch(int(shadowRes.y)) {
			case 0: cascadeColor = vec3(1.0, 0.0, 0.0); break;
			case 1: cascadeColor = vec3(0.0, 1.0, 0.0); break;
			case 2: cascadeColor = vec3(0.0, 0.0, 1.0); break;
			case 3: cascadeColor = vec3(1.0, 1.0, 0.0); break;
			default: cascadeColor = vec3(1.0, 0.0, 1.0);
		}
		o_Color = vec4(mix(o_Color.xyz,cascadeColor,0.5), 1.0);
	}
}
#endif