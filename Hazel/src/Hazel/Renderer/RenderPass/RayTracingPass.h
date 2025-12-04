#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class RayTracingPass : public RenderPassNew
	{
	public:
		RayTracingPass() = default;
		~RayTracingPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "RayTracingPass"; }
		virtual PassType GetType() override final { return RAYTRACING_PASS; }
	private:
		RHIShaderRef m_RayGenShader;
		RHIShaderRef m_ClosestHitShader;
		RHIShaderRef m_MissShader;


		RHIRootSignatureRef m_RootSignature;
		RHIRayTracingPipelineRef m_Pipeline;
	};
}


