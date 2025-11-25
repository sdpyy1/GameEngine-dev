#include "hzpch.h"
#include "MeshPass.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#define MAX_PER_PASS_PIPELINE_STATE_COUNT 1024      //每个mesh pass支持的最大的不同管线状态数目

namespace GameEngine
{
	void MeshPassProcessor::Init()
	{

	}

	// drawBatches从场景中收集
	void MeshPassProcessor::Process(const std::vector<DrawBatch>& drawBatches)
	{
		// 1.处理场景的drawBatch数据，收集到m_Batches中
		m_Batches.clear();
		for (auto& batch : drawBatches)
		{
			OnCollectBatch(batch);  // 具体的Pass重载逻辑
		}

		// 2. 按管线进行分类
		for (auto& batch : m_Batches)
		{
			OnBuildDrawInfo(batch);
		}

		// 到这里，这个Pass需要的模型数据已经按照PipelineState分类好了~

		// 3. 创建或获取需要的Pipeline
		uint32_t pipelineIndex = 0;
		for (auto& pair : m_DrawGeometries)
		{
			ASSERT(pipelineIndex < MAX_PER_PASS_PIPELINE_STATE_COUNT);
			RHIGraphicsPipelineRef pipeline = OnCreatePipeline(pair.first);

			// 4. 构建最终的DrawCommands
			if (pipeline)
			{
				OnBuildDrawCommands(pipelineIndex, pipeline, pair.second);
				pipelineIndex++;
			}
		}

		// 5.将准备好的全部数据提交给GPU端
		V2::IndirectSetting meshDrawSetting;
		meshDrawSetting.processSize = (uint32_t)meshDrawInfos.size();
		meshDrawSetting.pipelineStateSize = pipelineIndex;
		meshDrawSetting.drawSize = 0;
		meshDrawSetting.frustumCull = 0;
		meshDrawSetting.occlusionCull = 0;
		
		auto buffers = GetIndirectBuffers();
		buffers->meshDrawDataBuffer.SetData(&meshDrawSetting, sizeof(V2::IndirectSetting), 0);
		buffers->meshDrawDataBuffer.SetData(meshDrawInfos.data(), meshDrawInfos.size() * sizeof(V2::IndirectMeshDrawInfo), sizeof(V2::IndirectSetting));
		buffers->meshDrawCommandBuffer.SetData(meshDrawCommands.data(), meshDrawCommands.size() * sizeof(RHIIndirectCommand), 0);

	}
	void MeshPassProcessor::Draw(RHICommandListRef command) {
		for (auto& drawCommand : drawCommands)
		{
			command->SetGraphicsPipeline(drawCommand.pipeline);

			if (drawCommand.meshCommandRange.size > 0)
			{
				command->DrawIndirect(
					drawCommand.indirectMeshCommandBuffer,
					drawCommand.meshCommandOffset + drawCommand.meshCommandRange.begin * sizeof(RHIIndirectCommand),
					drawCommand.meshCommandRange.size);
			}
		}
	}

	// 根据材质的管线信息，将DrawBatch分组
	void MeshPassProcessor::OnBuildDrawInfo(DrawBatch& batch)
	{
		// 根据batch的材质，构建管线信息
		DrawPipelineState pipelineState = {};
		pipelineState.renderQueue = batch.material->RenderQueue();
		pipelineState.cullMode = batch.material->CullMode();
		pipelineState.fillMode = batch.material->GetFillMode();
		pipelineState.depthCompare = batch.material->DepthCompare();
		pipelineState.depthTest = batch.material->DepthTest();
		pipelineState.depthWrite = batch.material->DepthWrite();
		pipelineState.vertexShader = batch.material->GetVertexShader() ? batch.material->GetVertexShader()->GetRHIShader() : nullptr;
		pipelineState.geometryShader = batch.material->GetGeometryShader() ? batch.material->GetGeometryShader()->GetRHIShader() : nullptr;
		pipelineState.fragmentShader = batch.material->GetFragmentShader() ? batch.material->GetFragmentShader()->GetRHIShader() : nullptr;
		// pipelineState.clusterRender = (batch.clusterGroupID.begin > 0 || batch.clusterID.begin > 0) ? true : false;
		pipelineState.clusterRender = false;
		pipelineState.meshRender = !pipelineState.clusterRender;

		DrawGeometryInfo info = {};
		info.objectID = batch.objectID;
		info.vertexID = batch.vertexBuffer->vertexID;
		info.indexID = batch.indexBuffer->indexID;
		info.indexCount = batch.indexBuffer->IndexNum();
		// info.clusterID = batch.clusterID;
		// info.clusterGroupID = batch.clusterGroupID;
		AddDrawInfo(pipelineState, info);
	}

	void MeshPassProcessor::AddDrawInfo(DrawPipelineState& pipelineState, DrawGeometryInfo info)
	{
		m_DrawGeometries[pipelineState].push_back(info);
	}

	RHIGraphicsPipelineRef MeshPassProcessor::OnCreatePipeline(const DrawPipelineState& pipelineState)
	{
		RHIGraphicsPipelineInfo pipelineInfo = {};
		pipelineInfo.vertexShader = pipelineState.vertexShader;
		pipelineInfo.geometryShader = pipelineState.geometryShader;
		pipelineInfo.fragmentShader = pipelineState.fragmentShader;
		pipelineInfo.primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
		pipelineInfo.rasterizerState = { pipelineState.fillMode, pipelineState.cullMode, DEPTH_CLIP, 0.0f, 0.0f };
		pipelineInfo.depthStencilState = { pipelineState.depthCompare, pipelineState.depthTest, pipelineState.depthWrite };

		// 需要各个mesh pass 重载，提供根签名和帧缓冲信息
		// pipelineInfo.rootSignature                   = rootSignature;
		// for(uint32_t i = 0; i < 4; i++) pipelineInfo.blendState.renderTargets[i].enable = false;
		// pipelineInfo.colorAttachmentFormats[0]      = FORMAT_R8G8B8A8_UNORM;
		// pipelineInfo.colorAttachmentFormats[1]      = FORMAT_R8G8B8A8_UNORM;
		// pipelineInfo.colorAttachmentFormats[2]      = FORMAT_R8G8B8A8_UNORM;
		// pipelineInfo.colorAttachmentFormats[3]      = FORMAT_R16G16B16A16_SFLOAT;                                               
		// pipelineInfo.depthStencilAttachmentFormat   = EngineContext::Render()->GetDepthFormat();

		return GraphicsPipelineCache::Get()->Allocate(pipelineInfo).pipeline;   // 构建可能失败返回空，则后续处理将放弃该pipelineState的绘制
	}

	void MeshPassProcessor::OnBuildDrawCommands(uint32_t pipelineIndex, RHIGraphicsPipelineRef pipeline, std::vector<DrawGeometryInfo>& geometries)
	{
		auto buffers = GetIndirectBuffers();
		DrawCommand drawCommand;
		drawCommand.pipeline = pipeline;
		drawCommand.meshCommandRange = { (uint32_t)meshDrawCommands.size(), 0 };
		drawCommand.indirectMeshCommandBuffer = buffers->meshDrawCommandBuffer.GetRHIBuffer();

		uint32_t meshCount = 0;
		for (auto& geometry : geometries) {
			V2::IndirectMeshDrawInfo meshDrawInfo;

			meshDrawInfo.objectID = geometry.objectID;
			meshDrawInfo.commandID = (uint32_t)meshDrawCommands.size();
			meshDrawInfos.push_back(meshDrawInfo);

			RHIIndirectCommand meshDrawCommand;
			meshDrawCommand.vertexCount = geometry.indexCount;
			meshDrawCommand.instanceCount = 1;                     // TODO 使用同一个顶点和索引缓冲的还能进一步合并？
			meshDrawCommand.firstVertex = 0;                       // 间接绘制里这样的多个indirect command 有多大的开销？
			meshDrawCommand.firstInstance = geometry.objectID;
			meshDrawCommands.push_back(meshDrawCommand);
			meshCount++;
		}
	}

	std::shared_ptr<MeshPassIndirectBuffers> MeshPassProcessor::GetIndirectBuffers()
	{
		return indirectBuffers[APP_FRAMEINDEX];
	}

}