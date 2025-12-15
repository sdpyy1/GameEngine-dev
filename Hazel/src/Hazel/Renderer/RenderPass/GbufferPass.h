#pragma once
#include "MeshPass.h"
namespace GameEngine
{
	class GBufferPass : public MeshPass
	{
	public:
		GBufferPass() = default;
		~GBufferPass() = default;


		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() override final { return "G-Buffer"; }

		virtual PassType GetType() override final { return GBUFFER_PASS; }
	private:
		RHIRootSignatureRef rootSignature;

		ShaderRef vertexShader;
		ShaderRef clusterVertexShader;
		ShaderRef fragmentShader;
		RHIBufferRef settingBuffer;
		RHIGraphicsPipelineRef pipeline;
		RHIGraphicsPipelineRef clusterPipeline;
		friend class GBufferPassProcessor;

	};

	class GBufferPassProcessor : public MeshPassProcessor
	{
	public:
		GBufferPassProcessor(GBufferPass* pass) { this->pass = pass; }

		virtual void AddMeshBatch(const MeshBatch& batch) override final;
		virtual RHIGraphicsPipelineRef OnCreatePipeline(const DrawPipelineState& pipelineState) override final;
	private:
		GBufferPass* pass;
	};
}

