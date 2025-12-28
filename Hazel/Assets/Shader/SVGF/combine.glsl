#version 450 core
#include "../common/common.glsl"
#include "../common/postprocess.glsl"

#ifdef COMPUTE_SHADER

layout(set = 1, binding = 0, rgba32f) uniform image2D OUTCOLOR;
layout(set = 1, binding = 1, rgba32f) uniform image2D directCOLOR;
layout(set = 1, binding = 2, rgba32f) uniform image2D inDirectCOLOR;
layout(set = 1, binding = 3, rgba32f) uniform image2D albedo;
layout(set = 1, binding = 4) readonly buffer EXPOSURE
{
    ExposureSetting setting;     
    float luminance;                     // 计算得到的平均亮度
    float adaptedLuminance;              // 多帧渐进的亮度
    float _padding[2];   
    uint histogramBuffer[256];           // 直方图数组 
    uint readBackHistogramBuffer[256];   // 回读用的数组 
}EXPOSURE_DATA;
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

void main() 
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);

    // 读取输入颜色
    vec3 direct = imageLoad(directCOLOR, pixel).xyz;
    vec3 indirect = imageLoad(inDirectCOLOR, pixel).xyz;
    vec3 a = imageLoad(albedo, pixel).xyz;

    // 计算输出
    vec3 outColor = (direct + indirect) * a;



    ColorSetting ColorSETTING = GetColorSetting();
	float finalExposure = ColorSETTING.exposure / EXPOSURE_DATA.adaptedLuminance;

	// Tone Mapping
	if(ColorSETTING.toneMappingMode == 0) outColor = CEToneMapping(outColor, finalExposure); 
	else if(ColorSETTING.toneMappingMode == 1) outColor = Uncharted2ToneMapping(outColor, finalExposure); 
	else if(ColorSETTING.toneMappingMode == 2) outColor = ACESToneMapping(outColor, finalExposure); 
	
	//饱和度
	outColor = SaturationColor(outColor, ColorSETTING.saturation);

	// 对比度
	outColor = ContrastColor(outColor, ColorSETTING.contrast);

	// TODO: Grading

	// Gamma矫正
	const float gamma = 2.2;
	outColor = GammaCorrect(outColor, gamma);



    // 写入输出纹理
    imageStore(OUTCOLOR, pixel, vec4(outColor, 1.0));
}

#endif
