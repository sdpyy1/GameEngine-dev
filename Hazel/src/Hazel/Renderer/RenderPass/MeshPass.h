#pragma once
/*
UE的MeshPass: 
PrimitiveSceneProxy(场景数据)->FMeshBatch(收集的Mesh数据)->FMeshPassProcessor(每个MeshPass都有一个Processor来按照自己的规则处理MeshBatch)->每个Pass生成自己的FMeshDrawCommand->RHI




*/
namespace GameEngine { 
	class MeshPass
	{
	};



	// 每个MeshPass都有自己的Processor，用于按自己的方式处理MeshBatch
	class MeshPassProcessor
	{
		
	};
}
