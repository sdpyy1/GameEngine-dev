#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class LightPass : public RenderPassNew
	{
	public:
		LightPass() = default;
		~LightPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "LightPass"; }
		virtual PassType GetType() override final { return LIGHT_PASS; }
	private:
		RHIShaderRef m_VertShader;
		RHIShaderRef m_FragShader;
		RHIRootSignatureRef m_RootSignature;
		RHIGraphicsPipelineRef m_Pipeline;
	};
}


