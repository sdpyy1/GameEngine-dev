#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class TAAPass : public RenderPass
	{
	public:
		TAAPass() = default;
		~TAAPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "TAAPass"; }
		virtual PassType GetType() override final { return BLOOM_PASS; }
	private:
		RHIShaderRef m_Shader;
		RHIRootSignatureRef m_RootSignature;
		RHIComputePipelineRef m_Pipeline;
		bool isFirstTick = true;
		RHITextureRef historyTexture;
	};
}
