#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class SVGFPass : public RenderPass
	{
	public:
		SVGFPass() = default;
		~SVGFPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "SVGFPass"; }
		virtual PassType GetType() override final { return SVGF_PASS; }
	private:
		RHIShaderRef m_Shader;
		RHIRootSignatureRef m_RootSignature;
		RHIComputePipelineRef m_Pipeline;

		RHITextureRef m_directFilterHistory;
		RHITextureRef m_indirectFilterHistory;

	};
}


