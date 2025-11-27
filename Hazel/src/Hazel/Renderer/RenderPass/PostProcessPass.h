#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class PostProcessPass : public RenderPassNew
	{
	public:
		PostProcessPass() = default;
		~PostProcessPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "PostProcessPass"; }
		virtual PassType GetType() override final { return SKY_PASS; }
	private:
		RHIShaderRef m_VertShader;
		RHIShaderRef m_FragShader;
		RHIRootSignatureRef m_RootSignature;
		RHIGraphicsPipelineRef m_Pipeline;
	};
}


