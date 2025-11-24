#include "hzpch.h"
#include "MeshPass.h"

namespace GameEngine
{
	void MeshPassProcessor::Init()
	{

	}

	void MeshPassProcessor::Process(const std::vector<DrawBatch>& drawBatches)
	{
		// 收集DrawBatch
		m_Batches.clear();
		for (auto& batch : drawBatches)
		{
			OnCollectBatch(batch);  // 具体的Pass重载逻辑
		}

	
		uint32_t pipelineIndex = 0;





	}



}