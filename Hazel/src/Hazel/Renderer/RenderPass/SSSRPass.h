#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class SSSRPass : public RenderPass
	{
	public:
		SSSRPass() = default;
		~SSSRPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "SSSRPass"; }
		virtual PassType GetType() override final { return SSSR_PASS; }
	private:
		RHIShaderRef m_Shader;
		RHIRootSignatureRef m_RootSignature;
		RHIComputePipelineRef m_Pipeline;
	};
}


