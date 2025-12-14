#pragma once
#include "RenderPass.h"

namespace GameEngine {
	class PostProcessPass : public RenderPass
	{
	public:
		PostProcessPass() = default;
		~PostProcessPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "PostProcessPass"; }
		virtual PassType GetType() override final { return SKY_PASS; }
	private:
		struct PostProcessingSetting
		{
			float exposure = 0.4f;
			float saturation = 1.0f;
			float contrast = 1.0f;
			uint32_t ToneMappingMode = 0;
		} m_Setting;
		RHIShaderRef m_VertShader;
		RHIShaderRef m_FragShader;
		RHIRootSignatureRef m_RootSignature;
		RHIGraphicsPipelineRef m_Pipeline;
	};
}


