#include "hzpch.h"
#include "MeshPass.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"

namespace GameEngine
{
	void MeshPassProcessor::Init(CullingType PassType,uint32_t index)
	{
        m_PassType = PassType;
		m_Index = index;
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
			m_MeshIndirectDrawDataBuffer[i] = std::make_shared<RenderBuffer<MeshIndirectDrawData>>(RESOURCE_TYPE_RW_BUFFER | RESOURCE_TYPE_INDIRECT_BUFFER);
		}
	}

	// 在UE中各个Pass执行这个过程是并行的
	void MeshPassProcessor::Process(const std::vector<MeshBatch>& drawBatches)
	{
		m_MeshBatches.clear();
		m_MeshDrawCommands.clear();
		m_IndirectCommands.clear();
		m_MeshBatchMap.clear();
		// 1. 收集当前Pass需要的MeshBatch
		for (auto& batch : drawBatches)
		{
			AddMeshBatch(batch);
		}

		// 2. 按管线进行分类
		for (auto& batch : m_MeshBatches)
		{
			MapMeshBatches(batch);
		}

		// 3. 创建或获取需要的Pipeline
		uint32_t pipelineIndex = 0;
		for (auto& pair : m_MeshBatchMap)
		{
			ASSERT(pipelineIndex < MAX_PER_PASS_PIPELINE_STATE_COUNT);
			RHIGraphicsPipelineRef pipeline = OnCreatePipeline(pair.first);

			// 4. 构建最终的DrawCommands
			if (pipeline)
			{
				OnBuildDrawCommands(pipeline, pair.second);
				pipelineIndex++;
			}
		}

		// 5.将准备好的全部数据提交给GPU端
		uint32_t instanceCount = (uint32_t)m_MeshBatches.size();
		m_MeshIndirectDrawDataBuffer[APP_FRAMEINDEX]->SetData(&instanceCount, sizeof(uint32_t),0);
		m_MeshIndirectDrawDataBuffer[APP_FRAMEINDEX]->SetData(&m_PassType, sizeof(uint32_t), sizeof(uint32_t));
		m_MeshIndirectDrawDataBuffer[APP_FRAMEINDEX]->SetData(&m_Index, sizeof(uint32_t), 2*sizeof(uint32_t));
		m_MeshIndirectDrawDataBuffer[APP_FRAMEINDEX]->SetData(m_IndirectCommands.data(), instanceCount * sizeof(RHIIndirectCommand), 4*sizeof(uint32_t));
	}
	void MeshPassProcessor::Draw(RHICommandListRef command) {
		for (auto& drawCommand : m_MeshDrawCommands)
		{
			auto [w, h] = APP_WINDOWSIZE;

			command->SetGraphicsPipeline(drawCommand.pipeline);

			if (drawCommand.meshCommandRange.size > 0)
			{
				command->DrawIndirect(
					m_MeshIndirectDrawDataBuffer[APP_FRAMEINDEX]->GetRHIBuffer(),
					4 * sizeof(uint32_t) + drawCommand.meshCommandRange.begin * sizeof(RHIIndirectCommand),
					drawCommand.meshCommandRange.size);
			}
		}
	}

	void MeshPassProcessor::OnBuildDrawCommands(RHIGraphicsPipelineRef pipeline, std::vector<MeshBatch>& meshBatch)
	{
		MeshDrawCommand drawCommand;
        drawCommand.pipeline = pipeline;
		drawCommand.meshCommandRange = { (uint32_t)m_IndirectCommands.size(), 0 };
		for (auto& batch : meshBatch) { // 收集这个PSO的绘制命令到m_IndirectCommands（最终一并上传）
			RHIIndirectCommand meshDrawCommand;
			meshDrawCommand.firstInstance = batch.instanceID;   // 在Shader中通过这个拿到实例ID
            meshDrawCommand.vertexCount = batch.indexCount;
            meshDrawCommand.instanceCount = 1;
            meshDrawCommand.firstVertex = 0;

			m_IndirectCommands.push_back(meshDrawCommand);
		}
		drawCommand.meshCommandRange.size = (uint32_t)meshBatch.size();
		m_MeshDrawCommands.push_back(drawCommand);
	}

	void MeshPassProcessor::MapMeshBatches(MeshBatch& batch)
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

		m_MeshBatchMap[pipelineState].push_back(batch);
	}
	void MeshPass::Init()
	{
		int index = 0;
		if (GetType() == DIR_SHADOW_PASS) {
			for (auto& processsor : meshPassProcessors) {
				processsor->Init(CULLING_TYPE_DIRECTIONLIGHT_SHADOW,index++);
			}
		}
		else if (GetType() == POINT_SHADOW_PASS) {
			for (auto& processsor : meshPassProcessors) {
				processsor->Init(CULLING_TYPE_POINTLIGHT_SHADOW, index++);
			}
		}
		else {
            meshPassProcessors[0]->Init(CULLING_TYPE_BASE,0); // 普通的只需要一个就行
		}
	}
}