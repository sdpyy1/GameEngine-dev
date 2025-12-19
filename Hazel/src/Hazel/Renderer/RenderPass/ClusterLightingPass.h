#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class ClusterLightingPass : public RenderPass
	{
	public:
		ClusterLightingPass() = default;
		~ClusterLightingPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "ClusterLightingPass"; }
		virtual PassType GetType() override final { return CLUSTER_LIGHTING_PASS; }
	private:
		RHIShaderRef m_Shader;
		RHIRootSignatureRef m_RootSignature;
		RHIComputePipelineRef m_Pipeline;
	};
}


