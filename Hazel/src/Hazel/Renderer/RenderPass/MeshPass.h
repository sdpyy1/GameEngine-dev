#pragma once
#include "RenderPass.h"
#include <Hazel/Renderer/RenderResource/Material.h>
#include "Hazel/Utils/IndexAllocator.h"
/*
UE的MeshPass: 
PrimitiveSceneProxy(场景数据)->FMeshBatch(收集的Mesh数据)->FMeshPassProcessor(每个MeshPass都有一个Processor来按照自己的规则处理MeshBatch)->每个Pass生成自己的FMeshDrawCommand->RHI




*/
namespace GameEngine {

	struct DrawBatch
	{
		uint32_t objectID;                                          // 物体唯一索引

		// VertexBufferRef vertexBuffer;                               // 若不启用cluster和virtual mesh渲染，则为正常的顶点和索引缓冲
		// IndexBufferRef indexBuffer;                                 // 否则为合并后的cluster组

		// IndexRange clusterID = { 0, 0 };               // 若提交时begin不为0，则启用cluster渲染
		//IndexRange clusterGroupID = { 0, 0 };          // 若提交时begin不为0，则启用virtual mesh渲染

		MaterialRef material;                                       // 包含了材质数据的内存块，也包含了着色器信息

	};

	struct DrawPipelineState
	{
		uint32_t renderQueue;

		RHIShaderRef vertexShader;
		RHIShaderRef geometryShader;
		RHIShaderRef fragmentShader;

		RasterizerCullMode cullMode;
		RasterizerFillMode fillMode;
		CompareFunction depthCompare;
		bool depthTest;
		bool depthWrite;

		bool meshRender;
		bool clusterRender;


		friend bool operator== (const DrawPipelineState& a, const DrawPipelineState& b)
		{
			return  a.renderQueue == b.renderQueue &&
				a.cullMode == b.cullMode &&
				a.fillMode == b.fillMode &&
				a.depthTest == b.depthTest &&
				a.depthWrite == b.depthWrite &&
				a.depthCompare == b.depthCompare &&
				a.meshRender == b.meshRender &&
				a.clusterRender == b.clusterRender &&
				a.vertexShader.get() == b.vertexShader.get() &&
				a.geometryShader.get() == b.geometryShader.get() &&
				a.fragmentShader.get() == b.fragmentShader.get();
		}

		bool operator< (const DrawPipelineState& other)const
		{
			return  (renderQueue != other.renderQueue) ? (renderQueue < other.renderQueue) :
				(cullMode != other.cullMode) ? (cullMode < other.cullMode) :
				(fillMode != other.fillMode) ? (fillMode < other.fillMode) :
				(depthTest != other.depthTest) ? (depthTest < other.depthTest) :
				(depthWrite != other.depthWrite) ? (depthWrite < other.depthWrite) :
				(depthCompare != other.depthCompare) ? (depthCompare < other.depthCompare) :
				(meshRender != other.meshRender) ? (meshRender < other.meshRender) :
				(clusterRender != other.clusterRender) ? (clusterRender < other.clusterRender) :
				(vertexShader.get() != other.vertexShader.get()) ? (vertexShader.get() < other.vertexShader.get()) :
				(geometryShader.get() != other.geometryShader.get()) ? (geometryShader.get() < other.geometryShader.get()) :
				(fragmentShader.get() != other.fragmentShader.get()) ? (fragmentShader.get() < other.fragmentShader.get()) : false;
		}

	};


	struct DrawCommand
	{
		RHIGraphicsPipelineRef pipeline;

		IndexRange meshCommandRange = { 0, 0 };
		uint32_t meshCommandOffset = 0;
		RHIBufferRef indirectMeshCommandBuffer;

		//IndexRange clusterCommandRange = { 0, 0 };
		//uint32_t clusterCommandOffset = 0;
		//RHIBufferRef indirectClusterCommandBuffer;
	};


	// 每个MeshPass都有自己的Processor，用于按自己的方式处理MeshBatch
	class MeshPassProcessor {
	public:
		void Init();
		void Process(const std::vector<DrawBatch>& drawBatches);
		virtual void MeshPassProcessor::OnCollectBatch(const DrawBatch& batch) = 0;   // 需要具体的Pass说明这个batch自己需不需要






	private:
		std::vector<DrawBatch> m_Batches;   // 从场景中收集的Mesh
		std::vector<DrawCommand> drawCommands;

	};
	using MeshPassProcessorRef = std::shared_ptr<MeshPassProcessor>;




	class MeshPass : public RenderPassNew
	{
	public:
		virtual void Init() override { meshPassProcessor->Init(); }
		virtual std::vector<MeshPassProcessorRef> GetMeshPassProcessors() { return { meshPassProcessor }; }

	protected:
		MeshPassProcessorRef meshPassProcessor = std::make_shared<MeshPassProcessor>();
	};
}
