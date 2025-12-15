#include "hzpch.h"
#include "MeshPass.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include "Hazel/Scene/SceneManager.h"

#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#define MAX_PER_PASS_PIPELINE_STATE_COUNT 1024      //每个mesh pass支持的最大的不同管线状态数目

namespace GameEngine
{
	void MeshPassProcessor::Init()
	{
		for (auto& indirectBuffer : indirectBuffers)
		{
			if (indirectBuffer == nullptr)
				indirectBuffer = std::make_shared<MeshPassIndirectBuffers>();
		}
	}

	// 在UE中各个Pass执行这个过程是并行的
	void MeshPassProcessor::Process(const std::vector<MeshBatch>& drawBatches)
	{
		m_Batches.clear();
		m_DrawGeometries.clear();
		drawCommands.clear();
		meshDrawCommands.clear();
		meshDrawInfos.clear();


		// 1. 收集当前Pass需要的MeshBatch
		for (auto& batch : drawBatches)
		{
			AddMeshBatch(batch);
		}

		// 2. 按管线进行分类
		for (auto& batch : m_Batches)
		{
			OnBuildDrawInfo(batch);
		}

		// 到这里，这个Pass需要的模型数据已经按照PipelineState分类好了~ 每个管线状态对应一组DrawGeometryInfo，每个DrawGeometryInfo（就是记录了绘制一个SubMesh需要的所有信息，因为是Bindless存储，所以都是一些ID）记录一个Mesh的MeshInfo位置（存储材质）顶点信息、索引信息

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
		IndirectSetting meshDrawSetting;
		meshDrawSetting.processSize = (uint32_t)meshDrawInfos.size();
		meshDrawSetting.pipelineStateSize = pipelineIndex;
		meshDrawSetting.drawSize = 0;
		meshDrawSetting.frustumCull = 0;
		meshDrawSetting.occlusionCull = 0;

		auto buffers = GetIndirectBuffers();
		buffers->meshDrawDataBuffer.SetData(&meshDrawSetting, sizeof(IndirectSetting), 0);
		buffers->meshDrawDataBuffer.SetData(meshDrawInfos.data(), meshDrawInfos.size() * sizeof(IndirectMeshDrawInfo), sizeof(IndirectSetting));
		buffers->meshDrawCommandBuffer.SetData(meshDrawCommands.data(), meshDrawCommands.size() * sizeof(RHIIndirectCommand), 0);
	}
	void MeshPassProcessor::Draw(RHICommandListRef command) {
		for (auto& drawCommand : drawCommands)
		{
			auto [w, h] = APP_WINDOWSIZE;

			// command->SetGraphicsPipeline(drawCommand.pipeline);

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
	void MeshPassProcessor::OnBuildDrawInfo(MeshBatch& batch)
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
		info.instanceID = batch.instanceID;
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

	void MeshPassProcessor::OnBuildDrawCommands(uint32_t pipelineIndex, RHIGraphicsPipelineRef pipeline, std::vector<DrawGeometryInfo>& geometries)
	{
		auto buffers = GetIndirectBuffers();

		DrawCommand drawCommand;
		drawCommand.pipeline = pipeline;
		drawCommand.meshCommandRange = { (uint32_t)meshDrawCommands.size(), 0 };
		drawCommand.indirectMeshCommandBuffer = buffers->meshDrawCommandBuffer.GetRHIBuffer();

		// 遍历当前pipeline下所有需要绘制的SubMesh信息（DrawGeometryInfo）
		uint32_t meshCount = 0;
		for (auto& geometry : geometries) {
			IndirectMeshDrawInfo meshDrawInfo;

			meshDrawInfo.instanceID = geometry.instanceID;
			meshDrawInfo.commandID = (uint32_t)meshDrawCommands.size();
			meshDrawInfos.push_back(meshDrawInfo);

			RHIIndirectCommand meshDrawCommand;
			meshDrawCommand.vertexCount = geometry.indexCount;   // 顶点数设置的索引数，在Shader中用gl_VertexIndex来获取对应的索引值，所有这里虽然调用的是DrawIndirect，其实本质是DrawIndexedIndirect
			meshDrawCommand.instanceCount = 1;                     // TODO 使用同一个顶点和索引缓冲的还能进一步合并？
			meshDrawCommand.firstVertex = 0;                       // 间接绘制里这样的多个indirect command 有多大的开销？
			meshDrawCommand.firstInstance = geometry.instanceID;   // 渲染时，通过实例索引来拿到对应的MeshInfo
			meshDrawCommands.push_back(meshDrawCommand);
			meshCount++;
		}
		drawCommand.meshCommandRange.size = meshCount;
		AddDrawCommand(drawCommand);
	}

	std::shared_ptr<MeshPassIndirectBuffers> MeshPassProcessor::GetIndirectBuffers()
	{
		return indirectBuffers[APP_FRAMEINDEX];
	}
}