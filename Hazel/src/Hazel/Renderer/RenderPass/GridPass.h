#pragma once
#include "RenderPass.h"
namespace GameEngine {
	class GridPass : public RenderPass
	{
	public:
		GridPass() = default;
		~GridPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "GridPass"; }
		virtual PassType GetType() override final { return GRID_PASS; }
	private:
		RHIShaderRef m_VertShader;
		RHIShaderRef m_FragShader;
		RHIRootSignatureRef m_RootSignature;
		RHIGraphicsPipelineRef m_Pipeline;
	};
}

