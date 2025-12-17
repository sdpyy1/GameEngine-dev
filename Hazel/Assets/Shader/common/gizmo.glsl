#ifndef GIZMO_GLSL
#define GIZMO_GLSL
#include "struct.glsl"
void AddGizmoBox(vec3 center, vec3 extent, vec4 color)
{
    GizmoBoxInfo info;
    info.center = center;
    info.extent = extent;
    info.color = color;

    uint offset = atomicAdd(GIZMO_DRAW_DATA.command[0].instanceCount, 1);   // ʵ��������1��ע��atomicAdd���ص��Ǿ�ֵ�����Ǽ�1�Ľ��
    if(offset < MAX_GIZMO_PRIMITIVE_COUNT) GIZMO_DRAW_DATA.boxes[offset] = info;
}
void AddGizmoBoundingBox(BoundingBox box, vec4 color){
        vec3 center = (box.maxBound + box.minBound) * 0.5;
        vec3 extent = (box.maxBound - box.minBound) * 0.5;
        AddGizmoBox(center, extent, color);
}
void AddGizmoSphere(vec3 center, float radious, vec4 color)
{
    GizmoSphereInfo info;
    info.center = center;
    info.radious = radious;
    info.color = color;

    uint offset = atomicAdd(GIZMO_DRAW_DATA.command[1].instanceCount, 1);
    if(offset < MAX_GIZMO_PRIMITIVE_COUNT) GIZMO_DRAW_DATA.spheres[offset] = info;
}

void AddGizmoLine(vec3 from, vec3 to, vec4 color)
{
    GizmoLineInfo info;
    info.from = from;
    info.to = to;
    info.color = color;

    uint offset = atomicAdd(GIZMO_DRAW_DATA.command[2].instanceCount, 1);
    if(offset < MAX_GIZMO_PRIMITIVE_COUNT) GIZMO_DRAW_DATA.lines[offset] = info;
}

void AddGizmoBillboard(vec3 center, vec2 extent, uint textureID, vec4 color)
{
    GizmoBillboardInfo info;
    info.center = center;
    info.extent = extent;
    info.textureID = textureID;
    info.color = color;

    uint offset = atomicAdd(GIZMO_DRAW_DATA.command[3].instanceCount, 1);
    if(offset < MAX_GIZMO_PRIMITIVE_COUNT) GIZMO_DRAW_DATA.worldBillboards[offset] = info;
}
#endif