#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class GPUCullingPass : public RenderPass
	{
	public:
		GPUCullingPass() = default;
		~GPUCullingPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "GPUCullingPass"; }
		virtual PassType GetType() override final { return GPUCULLING_PASS; }
	private:
		enum CullingPassType:uint32_t{
            CULLING_PASS_BASE = 0,
            CULLING_PASS_DIRECTION_SHADOW,
            CULLING_PASS_POINT_SHADOW
		};
		

		RHIShaderRef m_Shader;
		RHIRootSignatureRef m_RootSignature;
		RHIComputePipelineRef m_Pipeline;
	};
}


