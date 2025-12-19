#ifndef GIZMO_GLSL
#define GIZMO_GLSL
#include "struct.glsl"
void AddGizmoBox(vec3 center, vec3 extent, vec4 color)
{
    GizmoBoxInfo info;
    info.center = center;
    info.extent = extent;
    info.color = color;

    uint offset = atomicAdd(GIZMO_DRAW_DATA.command[0].instanceCount, 1); 
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

void DrawFrustumEdges(vec3 p0, vec3 p1, vec3 p2, vec3 p3, vec3 p4, vec3 p5, vec3 p6, vec3 p7, vec4 color) {
    // 1. 绘制近平面的4条边（p0-p1, p1-p3, p3-p2, p2-p0）
    AddGizmoLine(p0, p1, color);
    AddGizmoLine(p1, p3, color);
    AddGizmoLine(p3, p2, color);
    AddGizmoLine(p2, p0, color);

    // 2. 绘制远平面的4条边（p4-p5, p5-p7, p7-p6, p6-p4）
    AddGizmoLine(p4, p5, color);
    AddGizmoLine(p5, p7, color);
    AddGizmoLine(p7, p6, color);
    AddGizmoLine(p6, p4, color);

    // 3. 绘制连接近远平面的4条边（p0-p4, p1-p5, p2-p6, p3-p7）
    AddGizmoLine(p0, p4, color);
    AddGizmoLine(p1, p5, color);
    AddGizmoLine(p2, p6, color);
    AddGizmoLine(p3, p7, color);
}

#endif