vec4 ImportanceSampleGGX( vec2 E, float a2 )
{
	float Phi = 2 * PI * E.x;
	float CosTheta = sqrt( (1 - E.y) / ( 1 + (a2 - 1) * E.y ) );
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );

	vec3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;
	
	float d = ( CosTheta * a2 - CosTheta ) * CosTheta + 1;
	float D = a2 / ( PI*d*d );
	float PDF = D * CosTheta;	// 用法线分布来对环境光照进行采样时的使用的概率密度函数，按法线分布函数的定义，D * CosTheta半球积分就是1

	return vec4( H, PDF );	// 这里的PDF是半程向量的，反射向量的需要用下面的函数转一遍
}
