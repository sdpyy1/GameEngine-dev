#version 450 core
#include "../common/common.glsl"
#ifdef VERTEX_SHADER
layout(location = 0) in vec3 IN_POS;
layout(location = 0) out vec4 OUT_COLOR;

void main() 
{
    uint objectID               = gl_InstanceIndex;
    GizmoSphereInfo info   = GIZMO_DRAW_DATA.spheres[objectID];

    OUT_COLOR                   = info.color;

    vec4 pos = vec4(IN_POS * info.radious + info.center, 1.0f);
    gl_Position = GetCamera().projNoJetter * GetCamera().view * pos;
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) in vec4 IN_COLOR;
layout (location = 0) out vec4 OUT_COLOR;

void main()
{
    OUT_COLOR = IN_COLOR;
}
#endif
