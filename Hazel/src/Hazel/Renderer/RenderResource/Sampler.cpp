#include "hzpch.h"
#include "Sampler.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderManager.h"

namespace GameEngine {
	Sampler::Sampler()
	{
		RHISamplerInfo samplerInof;
		samplerInof.minFilter = FILTER_TYPE_LINEAR;
		samplerInof.magFilter = FILTER_TYPE_LINEAR;
		samplerInof.mipmapMode = MIPMAP_MODE_LINEAR;
		samplerInof.addressModeU = ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInof.addressModeV = ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInof.addressModeW = ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInof.compareFunction = COMPARE_FUNCTION_NEVER;
		samplerInof.mipLodBias = 0.0f;
		samplerInof.maxAnisotropy = 0.0f;

		sampler = APP_DYNAMICRHI->CreateSampler(samplerInof);
	}

	Sampler::Sampler(AddressMode addressMode, FilterType filterType, MipMapMode mipmapMode, float maxAnisotropy, SamplerReductionMode reductionMode)
	{
		RHISamplerInfo samplerInfo;
		samplerInfo.minFilter = filterType;
		samplerInfo.magFilter = filterType;
		samplerInfo.mipmapMode = mipmapMode;
		samplerInfo.addressModeU = addressMode;
		samplerInfo.addressModeV = addressMode;
		samplerInfo.addressModeW = addressMode;
		samplerInfo.compareFunction = COMPARE_FUNCTION_NEVER;
		samplerInfo.reductionMode = reductionMode;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.maxAnisotropy = maxAnisotropy;
		sampler = APP_DYNAMICRHI->CreateSampler(samplerInfo);
	}
}