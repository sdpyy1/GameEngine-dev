#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class DDGIPass : public RenderPassNew
	{
	public:
		DDGIPass() = default;
		~DDGIPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "DDGIPass"; }
		virtual PassType GetType() override final { return DDGI_PASS; }
	private:
		bool isFirstTick = true;
		RHIShaderRef m_RayGenShader;
		RHIShaderRef m_ClosestHitShader;
		RHIShaderRef m_MissShader;
		RHIShaderRef m_ShadowMissShader;

		RHIShaderRef m_ProbeIrrandianceBlendShader;
        RHIShaderRef m_ProbeDistanceBlendShader;


		RHIRootSignatureRef m_VolumeTraceRootSignature;
		RHIRayTracingPipelineRef m_VolumeTracePipeline;


		RHIRootSignatureRef m_ProbeIrrandianceBlendRootSignature;
		RHIComputePipelineRef m_ProbeIrrandianceBlendPipeline;

        RHIRootSignatureRef m_ProbeDistanceBlendRootSignature;
		RHIComputePipelineRef m_ProbeDistanceBlendPipeline;


		RHITextureRef m_ProbeDistanceTexture;
        RHITextureRef m_ProbeIrrandianceTexture;

	};
}


