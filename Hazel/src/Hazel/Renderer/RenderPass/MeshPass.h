#pragma once
#include "RenderPass.h"
#include <Hazel/Renderer/RenderResource/Material.h>
#include "Hazel/Utils/IndexAllocator.h"
#include <Hazel/Renderer/RenderResource/RenderBuffer.h>
/*
UE的流程：
	1. 从FPrimitiveSceneProxy到FMeshBatch（主要包含顶点工厂 + 材质）
	2. 从FMeshBatch到FMeshDrawCommand（遍历EMeshPass定义的所有Pass，创建对应的FMeshPassProcessor处理这些FMeshBatch）（这些Pass的处理是并行进行的）（DrawCommand中装了PSO、Shader等这个DrawCall需要的信息）
*/


/*
	TODO: 目前实现没有考虑实例化合并，因为这样会使剔除变得复杂，因为剔除不会考虑连续性。中间的实例被剔除，就必须提供额外的手段来处理这种情况。
	整体流程:
	1. 收集MeshBatch
	2. 缓存需要的PSO，并将MeshBatch根据PSO进行分组
	3. 每个PSO对应的MeshBatchs组成一个MeshDrawCommand，通过间接渲染接口一口气全部上传（提前进行GPU剔除）(也就是说一口气上传了这个Pass需要的所有DrawCall,并记录了每个PSO的范围，后续执行间接渲染时，只需要根据范围就有找到对应的DrawCall)
*/

namespace GameEngine {
	/*
		MeshBatch：因为所有Mesh信息都使用了Bindless，所以MeshBatch只需要存储实例ID和材质，但是因为DrawCall需要指定渲染顶点数量，所以需要传入Mesh的索引数量（逻辑是让顶点着色器执行索引次数来手动装配三角形）
	*/
	struct MeshBatch
	{
		uint32_t instanceID;
		uint32_t indexCount;
		MaterialRef material;
	};

	// 用于给Batch按照PSO进行分组
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
	

	enum CullingType :uint32_t {
		CULLING_TYPE_BASE,
		CULLING_TYPE_DIRECTIONLIGHT_SHADOW,
		CULLING_TYPE_POINTLIGHT_SHADOW,
		CULLING_TYPE_MAX_CNT
	};

	/*
		为了方便剔除时拿到详细信息，上传的Buffer不能只包含绘制指令,还需要记录实例数量,如果后续需要更多信息，可以扩展这个结构体
	*/
	struct MeshIndirectDrawData {
		 uint32_t instanceCount;
		 CullingType passType;
		 uint32_t index;  // 定向光表示CSM，点光源表示id
		 uint32_t _padding;

		 std::array<RHIIndirectCommand, MAX_PER_FRAME_INSTANCE_SIZE> indirectCommands;
	};

	/*
		最终定义一次DrawCall需要的信息
		1. 使用的PSO
		2. 间接绘制Buffer的Batch（PSO一致的绘制指令可以一次性全部上传）
	*/
	struct MeshDrawCommand
	{
		RHIGraphicsPipelineRef pipeline;
		IndexRange meshCommandRange = { 0, 0 };
	};

	class MeshPassProcessor {
	public:
		void Init(CullingType PassType, uint32_t index);
		void Process(const std::vector<MeshBatch>& drawBatches);
		void Draw(RHICommandListRef command);
		void AddBatch(const MeshBatch& batch) { m_MeshBatches.push_back(batch); }
		void OnBuildDrawCommands(RHIGraphicsPipelineRef pipeline, std::vector<MeshBatch>& meshBatch);
		uint32_t GetDrawCommandCount() { return m_MeshBatches.size(); }
		RHIBufferRef GetMeshIndirectDrawDataBuffer() { return m_MeshIndirectDrawDataBuffer[APP_FRAMEINDEX]->GetRHIBuffer(); }
		std::vector<MeshBatch>& GetMeshBatches() { return m_MeshBatches; }
	protected:
		virtual void MeshPassProcessor::AddMeshBatch(const MeshBatch& batch) = 0;
		virtual RHIGraphicsPipelineRef OnCreatePipeline(const DrawPipelineState& first) = 0;
	private:
		void MapMeshBatches(MeshBatch& batch);
	private: // culling时用来判断这是什么pass的信息
		CullingType m_PassType;
		uint32_t m_Index;
	private:
		std::vector<MeshBatch> m_MeshBatches; // 收集当前Pass需要的batch
		std::array<std::shared_ptr<RenderBuffer<MeshIndirectDrawData>>, FRAMES_IN_FLIGHT> m_MeshIndirectDrawDataBuffer;
		std::map<DrawPipelineState, std::vector<MeshBatch>> m_MeshBatchMap;
		std::vector<MeshDrawCommand> m_MeshDrawCommands;  // 存储这个是为了Draw的时候遍历
		std::vector<RHIIndirectCommand> m_IndirectCommands; // 存储这个是为了把Commands一口气上传
	};
	using MeshPassProcessorRef = std::shared_ptr<MeshPassProcessor>;



	class MeshPass : public RenderPass
	{
	public:
		virtual void Init() override;
		virtual std::vector<MeshPassProcessorRef> GetMeshPassProcessors() { return meshPassProcessors; }

	protected:
		// 每个MeshPass用自己继承的MeshPassProcessor来初始化这个指针
		// MeshPassProcessorRef meshPassProcessor = nullptr;
		std::vector<MeshPassProcessorRef> meshPassProcessors;
	};
}
