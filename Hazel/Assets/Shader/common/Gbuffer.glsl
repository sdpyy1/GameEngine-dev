// 使用GBuffer时，需要空出Set=3
#define GBUFFER_POSITION_BINDING 0
#define GBUFFER_NORMAL_BINDING 1
#define GBUFFER_MATERIAL_BINDING 2
#define GBUFFER_ALBEDO_BINDING 3
layout(set = 3, binding = GBUFFER_POSITION_BINDING) uniform texture2D u_GBufferPosition;
layout(set = 3, binding = GBUFFER_NORMAL_BINDING) uniform texture2D u_GBufferNormal;
layout(set = 3, binding = GBUFFER_MATERIAL_BINDING) uniform texture2D u_GBufferMaterial;
layout(set = 3, binding = GBUFFER_ALBEDO_BINDING) uniform texture2D u_GBufferAlbedo;