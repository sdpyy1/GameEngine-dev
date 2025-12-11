#version 450 core
#ifdef COMPUTE_SHADER
#include "../common/common.glsl"
#include "../common/gizmo.glsl"

#define THREAD_SIZE_X 256
#define THREAD_SIZE_Y 1
#define THREAD_SIZE_Z 1
layout (local_size_x = THREAD_SIZE_X, local_size_y = THREAD_SIZE_Y, local_size_z = THREAD_SIZE_Z) in;
void main() 
{
	uint gID = gl_GlobalInvocationID.x;


    if(gID < LIGHTINFO.data.pointLightCount)
    {
        PointLight pointLight = LIGHTINFO.data.pointLights[gID];
        AddGizmoBillboard(pointLight.position, vec2(0.5f), GLOBAL_SETTING.data.iconTextures.pointLightID, vec4(pointLight.radiance, 1.0f));

        if(pointLight.showRadius == 1){
            AddGizmoSphere(pointLight.sphere.center, pointLight.sphere.radius, vec4(1.0, 0.0, 0.0, 1.0));
        }

    }

    if(gID < LIGHTINFO.data.spotLightCount)
    {
        SpotLight spotLight = LIGHTINFO.data.spotLights[gID];
        AddGizmoBillboard(spotLight.position, vec2(0.5f), GLOBAL_SETTING.data.iconTextures.spotLightID, vec4(spotLight.radiance, 1.0f));
        if(spotLight.showRange == 1){
            AddGizmoSphere(spotLight.position, spotLight.range, vec4(1.0, 0.0, 0.0, 1.0));
        }
    }

    if(gID == 0 && LIGHTINFO.data.dirLightCount != 0)
    {
        DirectionLight dirLight = LIGHTINFO.data.dirLights;
        
        AddGizmoBillboard(dirLight.position, vec2(0.5f), GLOBAL_SETTING.data.iconTextures.dirLightID, vec4(dirLight.radiance, 1.0f));

        if(dirLight.showDirection == 1){
           AddGizmoLine(dirLight.position,dirLight.position + (dirLight.direction * 3), vec4(1.0, 0.0, 0.0, 1.0));
        }
    }

    // 移动到DDGI中
    // if(gID == 0)
    // {
    //     DDGISetting ddgi = GetDDGISetting();

    //     if(ddgi.enable == 0 || ddgi.visulaize == 0) return; 
    //     vec3 center = ddgi.centerPosition;
    //     vec3 extent = (ddgi.box.maxBound - ddgi.box.minBound) * 0.5;
    //     AddGizmoBox(center, extent, vec4(0.0, 1.0, 0.0, 0.5));
    //     ivec3 probeCount = ivec3(ddgi.probeCount);
    //     vec3 step = ddgi.gridStep;
    //     vec3 startPos = center - extent + step * 0.5;
    //     float radius = 0.1f;
    //     for(int z = 0; z < probeCount.z; ++z)
    //     for(int y = 0; y < probeCount.y; ++y)
    //     for(int x = 0; x < probeCount.x; ++x)
    //     {
    //         vec3 probePos = startPos + vec3(x, y, z) * step;
    //         AddGizmoSphere(probePos, radius, vec4(1.0, 0.0, 0.0, 1.0));
    //     }
    // }
}

#endif
