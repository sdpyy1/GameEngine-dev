#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class SkyPass : public RenderPassNew
	{
	public:
		SkyPass() = default;
		~SkyPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "SkyPass"; }
		virtual PassType GetType() override final { return SKY_PASS; }
	private:

		RHIShaderRef TransmittanceLutShader;
		RHIRootSignatureRef TransmittanceLutRootSignature;
		RHIComputePipelineRef TransmittanceLutPipeline;

        RHIShaderRef MultiScatteringLutShader;
        RHIRootSignatureRef MultiScatteringLutRootSignature;
        RHIComputePipelineRef MultiScatteringLutPipeline;

        RHIShaderRef SkyViewLutShader;
        RHIRootSignatureRef SkyViewLutRootSignature;
        RHIComputePipelineRef SkyViewLutPipeline;


		RHIShaderRef m_VertShader;
		RHIShaderRef m_FragShader;
		RHIRootSignatureRef m_RootSignature;
		RHIGraphicsPipelineRef m_Pipeline;



		uint32_t TrasmittanceLutWidth = 256;
		uint32_t TrasmittanceLutHeight = 64;
		uint32_t MultiScatteringLutResolution = 32;
		uint32_t SkyViewLutWidth = 256;
		uint32_t SkyViewLutHeight = 128;
	};
}


