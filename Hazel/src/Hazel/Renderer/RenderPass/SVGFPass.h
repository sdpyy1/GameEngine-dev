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
		RHIShaderRef m_AtrousShader;
		RHIShaderRef m_VarianceShader;
		RHIShaderRef m_CombineShader;
		RHIShaderRef m_MixHistoryShader;
		RHIRootSignatureRef m_AtrousRootSignature;
		RHIRootSignatureRef m_VarianceRootSignature;
		RHIRootSignatureRef m_CombineRootSignature;
		RHIRootSignatureRef m_MixHistoryRootSignature;
		RHIComputePipelineRef m_AtrousPipeline;
		RHIComputePipelineRef m_CombinePipeline;
		RHIComputePipelineRef m_VariancePipeline;
		RHIComputePipelineRef m_MixHistoryPipeline;

		RHITextureRef m_DirVarianceHistory;
		RHITextureRef m_InDirVarianceHistory;
		RHITextureRef m_DirectHistory;
		RHITextureRef m_IndirectHistory;
		bool isFirstTick = true;
	};
}


