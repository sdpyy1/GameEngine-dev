#version 450 core
#ifdef COMPUTE_SHADER
#include "common/common.glsl"
#include "common/gizmo.glsl"

#define THREAD_SIZE_X 256
#define THREAD_SIZE_Y 1
#define THREAD_SIZE_Z 1
layout (local_size_x = THREAD_SIZE_X, local_size_y = THREAD_SIZE_Y, local_size_z = THREAD_SIZE_Z) in;

void main() 
{
	uint gID = gl_GlobalInvocationID.x;


    if(gID < u_LightInfo.data.pointLightCount)
    {
        PointLightInfo pointLight = u_LightInfo.data.pointLights[gID];
        AddGizmoBillboard(pointLight.position, vec2(0.5f), GLOBAL_SETTING.data.iconTextures.pointLightID, vec4(pointLight.radiance, 1.0f));

        if(pointLight.showRadius == 1){
            AddGizmoSphere(pointLight.sphere.center, pointLight.sphere.radius, vec4(1.0, 0.0, 0.0, 1.0));
        }

    }

    if(gID < u_LightInfo.data.spotLightCount)
    {
        SpotLightInfo spotLight = u_LightInfo.data.spotLights[gID];
        AddGizmoBillboard(spotLight.position, vec2(0.5f), GLOBAL_SETTING.data.iconTextures.spotLightID, vec4(spotLight.radiance, 1.0f));
        if(spotLight.showRange == 1){
            AddGizmoSphere(spotLight.position, spotLight.range, vec4(1.0, 0.0, 0.0, 1.0));
        }
    }

    if(gID == 0 && u_LightInfo.data.dirLightCount != 0)
    {
        DirLightInfo dirLight = u_LightInfo.data.dirLights;
        
        AddGizmoBillboard(dirLight.position, vec2(0.5f), GLOBAL_SETTING.data.iconTextures.dirLightID, vec4(dirLight.radiance, 1.0f));

        if(dirLight.showDirection == 1){
           AddGizmoLine(dirLight.position,dirLight.position + (dirLight.direction * 3), vec4(1.0, 0.0, 0.0, 1.0));
        }
    }
}

#endif
