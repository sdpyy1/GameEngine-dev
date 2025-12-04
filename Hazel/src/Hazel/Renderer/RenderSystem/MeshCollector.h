#pragma once
#include <Hazel/Renderer/RenderPass/MeshPass.h>

namespace GameEngine
{

	class MeshCollector
	{
	public:
		static void CollectMesh();

		static void Collect4TLAS(std::vector<RHIAccelerationStructureInstanceInfo>& instances);
	};

}
