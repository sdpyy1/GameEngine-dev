#pragma once
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

		virtual std::string GetName() { return "SkyPass"; }
		virtual PassType GetType() override final { return BLOOM_PASS; }
	private:
		struct bloomSetting
		{
			glm::vec4 Params; // (x) threshold, (y) threshold - knee, (z) knee * 2, (w) 0.25 / knee
			float LOD;
			int Mode;
		};
		RHIShaderRef m_Shader;
		RHIRootSignatureRef m_RootSignature;
		RHIComputePipelineRef m_Pipeline;
	};
}


