#version 450 core
#include "../common/common.glsl"
#include "../common/postprocess.glsl"

#ifdef VERTEX_SHADER
vec2 NDC[3] = vec2[](
    vec2(-1.0, -1.0), 
    vec2( 3.0, -1.0), 
    vec2(-1.0,  3.0) 
);
layout(location = 0) out vec2 out_texCoord;
void main(){

	gl_Position = vec4(NDC[gl_VertexIndex],0,1);
    out_texCoord = (NDC[gl_VertexIndex] + 1.0) * 0.5;
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) in vec2 in_texCoord;
layout(location = 0) out vec4 out_color;

layout(set = 2, binding = 0) uniform texture2D lightRes;
layout(set = 2, binding = 1) uniform texture2D BloomRes;
layout(set = 2, binding = 2) readonly buffer EXPOSURE_DATA
{
    ExposureSetting setting;     
    float luminance;                     // 计算得到的平均亮度
    float adaptedLuminance;              // 多帧渐进的亮度
    float _padding[2];   
    uint histogramBuffer[256];           // 直方图数组 
    uint readBackHistogramBuffer[256];   // 回读用的数组 
};
layout(set = 1, binding = 0) uniform sampler SAMPLER[];


void main(){
	ColorSetting SETTING = GetColorSetting();
	float BloomScale = GetBloomSetting().enable == 1?GetBloomSetting().bloomScale: 0;
	vec3 finalColor = texture(sampler2D(lightRes,SAMPLER[0]), in_texCoord).rgb;
	float finalExposure = SETTING.exposure / adaptedLuminance;

	// Bloom
	finalColor += texture(sampler2D(BloomRes,SAMPLER[0]), in_texCoord).rgb * BloomScale;
	
	// Tone Mapping
	if(SETTING.toneMappingMode == 0) finalColor = CEToneMapping(finalColor, finalExposure); 
	else if(SETTING.toneMappingMode == 1) finalColor = Uncharted2ToneMapping(finalColor, finalExposure); 
	else if(SETTING.toneMappingMode == 2) finalColor = ACESToneMapping(finalColor, finalExposure); 
	
	//饱和度
	finalColor = SaturationColor(finalColor, SETTING.saturation);

	// 对比度
	finalColor = ContrastColor(finalColor, SETTING.contrast);

	// TODO: Grading

	// Gamma矫正
	const float gamma = 2.2;
	finalColor = GammaCorrect(finalColor, gamma);
	out_color = vec4(finalColor, 1.0);
}

#endif
