#pragma once
#include "RenderPass.h"
#include "MeshPass.h"

namespace GameEngine {
	class PreDepthPass : public MeshPass
	{
	public:
		PreDepthPass() = default;
		~PreDepthPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "PreDepthPass"; }
		virtual PassType GetType() override final { return PREDEPTH_PASS; }
	private:
		RHIShaderRef m_VertShader;
		RHIShaderRef m_FragShader;
		RHIRootSignatureRef m_RootSignature;
		RHIGraphicsPipelineRef m_Pipeline;
		friend class PreDepthPassProcessor;
	};



	class PreDepthPassProcessor : public MeshPassProcessor
	{
	public:
		PreDepthPassProcessor(PreDepthPass* pass) { this->pass = pass; }

		virtual void OnCollectBatch(const DrawBatch& batch) override final;
		virtual RHIGraphicsPipelineRef OnCreatePipeline(const DrawPipelineState& pipelineState) override final;

	private:
		PreDepthPass* pass;
	};

}


