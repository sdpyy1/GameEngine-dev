#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class PathTracingPass : public RenderPassNew
	{
	public:
		PathTracingPass() = default;
		~PathTracingPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "PathTracingPass"; }
		virtual PassType GetType() override final { return PATHTRACING_PASS; }
	private:
		RHIShaderRef m_RayGenShader;
		RHIShaderRef m_ClosestHitShader;
		RHIShaderRef m_MissShader;

		RHITextureRef m_HistoryTexture;
		RHIRootSignatureRef m_RootSignature;
		RHIRayTracingPipelineRef m_Pipeline;
	};
}


