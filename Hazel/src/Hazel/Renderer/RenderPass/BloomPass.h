#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class BloomPass : public RenderPassNew
	{
	public:
		BloomPass() = default;
		~BloomPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "BloomPass"; }
		virtual PassType GetType() override final { return BLOOM_PASS; }
	private:
		struct bloomComputePushConstants
		{
			glm::vec4 Params; // (x) threshold, (y) threshold - knee, (z) knee * 2, (w) 0.25 / knee
			float LOD;
			int Mode;
		} m_BloomComputePushConstants;
		struct BloomSettings
		{
			bool Enabled = true;
			float Threshold = 1.0f;
			float Knee = 0.1f;
			float UpsampleScale = 1.0f;
			float Intensity = 1.0f;
			float DirtIntensity = 1.0f;
		}m_BloomSettings;
		uint32_t m_BloomComputeWorkgroupSize = 8;
		RHIShaderRef m_Shader;
		RHIRootSignatureRef m_RootSignature;
		RHIComputePipelineRef m_Pipeline;
	};
}


