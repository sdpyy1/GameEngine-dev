#ifndef POSTPROCESS_GLSL
#define POSTPROCESS_GLSL
struct ExposureSetting
{             
    float minLog2Luminance;             //最小亮度，对数
    float inverseLuminanceRange;        //亮度范围，对数倒数
    float luminanceRange;               //亮度范围，对数
    float numPixels;                    //总像素数
    float timeCoeff;                    //时间加权
    float _padding[3];
};

//CryEngine 2
vec3 CEToneMapping(vec3 color, float adapted_lum) 
{
    return vec3(1.0) - exp(-adapted_lum * color);
}

//Uncharted 2 - filmic tone mapping
vec3 F(vec3 x)
{
	const float A = 0.22f;
	const float B = 0.30f;
	const float C = 0.10f;
	const float D = 0.20f;
	const float E = 0.01f;
	const float F = 0.30f;
 
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 Uncharted2ToneMapping(vec3 color, float adapted_lum)
{
	const float WHITE = 11.2f;
	return F(1.6f * adapted_lum * color) / F(vec3(WHITE));
}

//ACES
vec3 ACESToneMapping(vec3 color, float adapted_lum)
{
	const float A = 2.51f;
	const float B = 0.03f;
	const float C = 2.43f;
	const float D = 0.59f;
	const float E = 0.14f;

	color *= adapted_lum;
	return (color * (A * color + B)) / (color * (C * color + D) + E);
}

//饱和度
vec3 SaturationColor(vec3 color, float saturation)
{
     vec3 luminanceWeighting = vec3(0.2125, 0.7154, 0.0721);
     float luminance = dot(color, luminanceWeighting);

     //灰度校准向量
     vec3 greyScaleColor = vec3(luminance);

     return mix(greyScaleColor, color, saturation);
}

//对比度
vec3 ContrastColor(vec3 color, float contrast)
{
    return (color - vec3(0.5)) * contrast + vec3(0.5);
}

vec3 GammaCorrect(vec3 color, float gamma)
{
	return pow(color, vec3(1.0f / gamma));
}
#endif