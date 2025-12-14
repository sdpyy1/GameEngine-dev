#pragma once
#include "RenderPass.h"
#include "MeshPass.h"

namespace GameEngine {
	class DirShadowPass : public MeshPass
	{
	public:
		DirShadowPass() = default;
		~DirShadowPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "DirShadowPass"; }
		virtual PassType GetType() override final { return DIR_SHADOW_PASS; }
	private:
		ShaderRef m_VertShader;
		ShaderRef m_FragShader;
		RHIRootSignatureRef m_RootSignature;
		RHIGraphicsPipelineRef m_Pipeline;
		friend class DirShadowPassProcessor;
	};



	class DirShadowPassProcessor : public MeshPassProcessor
	{
	public:
		DirShadowPassProcessor(DirShadowPass* pass) { this->pass = pass; }

		virtual void OnCollectBatch(const MeshBatch& batch) override final;
		virtual RHIGraphicsPipelineRef OnCreatePipeline(const DrawPipelineState& pipelineState) override final;

	private:
		DirShadowPass* pass;
	};

}


