#pragma once
#include "RenderPass.h"
#include <Hazel/Renderer/RenderResource/Material.h>
#include "Hazel/Utils/IndexAllocator.h"
#include <Hazel/Renderer/RenderResource/RenderBuffer.h>
/*
UE的MeshPass: 
PrimitiveSceneProxy(场景数据)->FMeshBatch(收集的Mesh数据)->FMeshPassProcessor(每个MeshPass都有一个Processor来按照自己的规则处理MeshBatch)->每个Pass生成自己的FMeshDrawCommand->RHI




*/
namespace GameEngine {

	struct DrawBatch
	{
		uint32_t objectID;                                          // 物体唯一索引

		VertexBufferRef vertexBuffer;                             
		IndexBufferRef indexBuffer;

		MaterialRef material;                                       // 包含了材质数据的内存块，也包含了着色器信息

	};

	// 渲染模型的材质才是决定Pipeline创建的依据
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
	typedef struct DrawGeometryInfo
	{
		uint32_t objectID; // meshInfo
		uint32_t vertexID; // vertexInfo
		uint32_t indexID; // indexID
		uint32_t indexCount; // 索引数量
		// IndexRange clusterID = { 0, 0 };
		// IndexRange clusterGroupID = { 0, 0 };

	} DrawGeometryInfo;
	typedef struct MeshPassIndirectBuffers
	{
		// 存储渲染需要的信息
		RenderBuffer<IndirectMeshDrawDatas> meshDrawDataBuffer;

		// 存储间接渲染需要的指令buffer的信息
		RenderBuffer<IndirectMeshDrawCommands> meshDrawCommandBuffer = RenderBuffer<IndirectMeshDrawCommands>(RESOURCE_TYPE_RW_BUFFER | RESOURCE_TYPE_INDIRECT_BUFFER);

	} MeshPassIndirectBuffers;

	struct DrawCommand  // 最终定义一次DrawCall的信息
	{
		RHIGraphicsPipelineRef pipeline;
		IndexRange meshCommandRange = { 0, 0 };
		uint32_t meshCommandOffset = 0;
		RHIBufferRef indirectMeshCommandBuffer;
	};


	// 每个MeshPass都有自己的Processor，用于按自己的方式处理MeshBatch
	class MeshPassProcessor {
	public:
		void Init();
		void Process(const std::vector<DrawBatch>& drawBatches);
		void Draw(RHICommandListRef command);
		void AddBatch(const DrawBatch& batch) { m_Batches.push_back(batch); }
		void AddDrawCommand(const DrawCommand& drawCommand) { drawCommands.push_back(drawCommand); }




	protected:
		virtual void MeshPassProcessor::OnCollectBatch(const DrawBatch& batch) = 0;   // 需要具体的Pass说明这个batch自己需不需要
		virtual RHIGraphicsPipelineRef OnCreatePipeline(const DrawPipelineState& first) = 0;  // 因为创建pipelien需要Shader信息也需要vkRenderPass也就是附件信息，这些需要具体的Pass提供（Shader也可以来自材质）


	private:
		std::vector<DrawBatch> m_Batches;   // 从场景中收集并处理过的每个SubMesh数据
		std::vector<DrawCommand> drawCommands;
		std::map<DrawPipelineState, std::vector<DrawGeometryInfo>> m_DrawGeometries; // 把渲染Batch按照PipelineState进行分类
		std::array<std::shared_ptr<MeshPassIndirectBuffers>, FRAMES_IN_FLIGHT> indirectBuffers;     // 每帧都完全重构的buffer，因此需要每帧一份   
		std::vector<RHIIndirectCommand> meshDrawCommands;
		std::vector<IndirectMeshDrawInfo> meshDrawInfos;

		void AddDrawInfo(DrawPipelineState& pipelineState, DrawGeometryInfo info);
		void OnBuildDrawInfo(DrawBatch& batch);
		void OnBuildDrawCommands(uint32_t pipelineIndex, RHIGraphicsPipelineRef pipeline, std::vector<DrawGeometryInfo>& second);
		std::shared_ptr<MeshPassIndirectBuffers> GetIndirectBuffers();
	};
	using MeshPassProcessorRef = std::shared_ptr<MeshPassProcessor>;




	class MeshPass : public RenderPassNew
	{
	public:
		virtual void Init() override { meshPassProcessor->Init(); }
		virtual std::vector<MeshPassProcessorRef> GetMeshPassProcessors() { return { meshPassProcessor }; }

	protected:
		MeshPassProcessorRef meshPassProcessor = nullptr;

	};
}
