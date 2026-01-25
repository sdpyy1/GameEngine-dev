#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class FXAAPass : public RenderPass
	{
	public:
		FXAAPass() = default;
		~FXAAPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "FXAAPass"; }
		virtual PassType GetType() override final { return FXAA_PASS; }
	private:
		RHIShaderRef m_Shader;
		RHIRootSignatureRef m_RootSignature;
		RHIComputePipelineRef m_Pipeline;
	};
}
