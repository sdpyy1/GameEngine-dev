#pragma once
#include "RHIBase.h"
#include "RHIResource.h"
#include <GLFW/glfw3.h>
namespace GameEngine {
		/* RHI总览
		* 上下文无关的操作，定义在DynamicRHI的虚函数，Init时进行各个API的初始化操作
		* 上下文相关的操作，定义在RHICommandContext的虚函数，创建RHICommandContext对象时，底层会创建一个上下文handle，比如Vulkan会创建一个VkCommandBuffer
		* 需要立即执行的命令，比如转换图片布局，定义在RHICommandContextImmediate，它需要上下文，但是需要Flush立即执行，底层也会创建一个上下文并且需要Fence控制并发
		*/
	class DynamicRHI {
	private:
		static DynamicRHIRef s_DynamicRHI;
	public:
		/*
			API Context
		*/
        static DynamicRHIRef Init(RHIConfig config);
		virtual void Tick();
		virtual RHISurfaceRef CreateSurface(GLFWwindow* window) = 0;
		virtual RHISwapchainRef CreateSwapChain(const RHISwapchainInfo& info) = 0;
		virtual RHICommandListImmediateRef GetImmediateCommandList() = 0;

		/*
			Render Context
		*/
		virtual RHICommandPoolRef CreateCommandPool(const RHICommandPoolInfo& info) = 0;
		virtual RHICommandContextRef CreateCommandContext(RHICommandPoolRef pool) = 0;
		virtual RHIRenderPassRef CreateRenderPass(const RHIRenderPassInfo& info) = 0;
		virtual RHIRootSignatureRef CreateRootSignature(const RHIRootSignatureInfo& info) = 0;
		virtual RHIComputePipelineRef CreateComputePipeline(const RHIComputePipelineInfo& info) = 0;
		virtual RHIGraphicsPipelineRef CreateGraphicsPipeline(const RHIGraphicsPipelineInfo& info) = 0;

		/*
			Texture and Buffer
		*/
		virtual RHITextureRef CreateTexture(const RHITextureInfo& info) = 0;
		virtual RHITextureViewRef CreateTextureView(const RHITextureViewInfo& info) = 0;
		virtual RHISamplerRef CreateSampler(const RHISamplerInfo& info) = 0;
		virtual RHIBufferRef CreateBuffer(const RHIBufferInfo& info) = 0;

		/*
			Shader
		*/
		virtual RHIShaderRef CreateShader(const RHIShaderInfo& info) = 0;

		/*
			Ray Tracing
		*/
		bool isEnableRayTracing() { return m_Config.enableRayTracing; }
		virtual RHIRayTracingPipelineRef CreateRayTracingPipeline(const RHIRayTracingPipelineInfo& info) = 0;
		virtual RHIShaderBindingTableRef CreateShaderBindingTable(const RHIShaderBindingTableInfo& info) = 0;
		virtual RHITopLevelAccelerationStructureRef CreateTopLevelAccelerationStructure(const RHITopLevelAccelerationStructureInfo& info) = 0;
		virtual RHIBottomLevelAccelerationStructureRef CreateBottomLevelAccelerationStructure(const RHIBottomLevelAccelerationStructureInfo& info) = 0;

		/*
			Fence and Semaphore
		*/
		virtual RHIFenceRef CreateFence(bool signaled) = 0;
		virtual RHISemaphoreRef CreateSemaphore() = 0;

		/*
			ImGUI
		*/
		virtual void InitImGui(GLFWwindow* window) = 0;
		virtual RHIDescriptorSetRef GetImGuiTextId(RHITextureViewRef textureView) = 0;

		/*
			MISC
		*/
		void RegisterResource(RHIResourceRef resource) { resourceMap[resource->GetType()].push_back(resource); }
		virtual RHIQueueRef GetQueue(const RHIQueueInfo& info) = 0;
		static DynamicRHIRef Get() { return s_DynamicRHI; }
		RHIConfig& GetConfig() { return m_Config; }


    protected:
		DynamicRHI(const RHIConfig& config) : m_Config(config) {};
		virtual ~DynamicRHI();
		std::array<std::vector<RHIResourceRef>, RHI_RESOURCE_TYPE_MAX_CNT> resourceMap;
        RHIConfig m_Config;
	};



	class RHICommandContext : public RHIResource {
	public:
		RHICommandContext(RHICommandPoolRef pool): RHIResource(RHI_COMMAND_CONTEXT), pool(pool){}
		virtual void BeginCommand() = 0;
		virtual void EndCommand() = 0;
		virtual void Execute(RHIFenceRef waitFence, RHISemaphoreRef waitSemaphore, RHISemaphoreRef signalSemaphore) = 0;
		virtual void TextureBarrier(const RHITextureBarrier& barrier) = 0;
		virtual void BufferBarrier(const RHIBufferBarrier& barrier) = 0;
		virtual void BeginRenderPass(RHIRenderPassRef renderPass) = 0;
		virtual void CopyTexture(RHITextureRef src, TextureSubresourceLayers srcSubresource, RHITextureRef dst, TextureSubresourceLayers dstSubresource) = 0;
		virtual void GenerateMips(RHITextureRef src) = 0;
		virtual void SetViewport(Offset2D min, Offset2D max) = 0;
		virtual void SetScissor(Offset2D min, Offset2D max) = 0;
		virtual void SetDepthBias(float constantBias, float slopeBias, float clampBias) = 0;
		virtual void SetLineWidth(float width) = 0;
		virtual void SetGraphicsPipeline(RHIGraphicsPipelineRef graphicsPipeline) = 0;
		virtual void SetComputePipeline(RHIComputePipelineRef computePipeline) = 0;
		virtual void PushLabel(const std::string& name, Color3 color = { 1.0f, 1.0f, 1.0f }) = 0;
		virtual void PopLabel() = 0;
		virtual void SetRayTracingPipeline(RHIRayTracingPipelineRef rayTracingPipeline) = 0;
		virtual void PushConstants(void* data, uint16_t size, ShaderFrequency frequency) = 0;
		virtual void BindDescriptorSet(RHIDescriptorSetRef descriptor, uint32_t set) = 0;
		virtual void BindVertexBuffer(RHIBufferRef vertexBuffer, uint32_t streamIndex, uint32_t offset) = 0;
		virtual void BindIndexBuffer(RHIBufferRef indexBuffer, uint32_t offset) = 0;
		virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
		virtual void DispatchIndirect(RHIBufferRef argumentBuffer, uint32_t argumentOffset) = 0;
		virtual void TraceRays(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
		virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
		virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) = 0;
		virtual void DrawIndirect(RHIBufferRef argumentBuffer, uint32_t offset, uint32_t drawCount) = 0;
		virtual void DrawIndexedIndirect(RHIBufferRef argumentBuffer, uint32_t offset, uint32_t drawCount) = 0;
		virtual void EndRenderPass() = 0;
		virtual std::vector<RHIGPUTimeInfo> GetGPUTime() = 0;
		virtual void ImGuiRenderDrawData() = 0;

	protected:
		RHICommandPoolRef pool;
	};

	class RHICommandContextImmediate : public RHIResource
	{
	public:
		RHICommandContextImmediate(): RHIResource(RHI_COMMAND_CONTEXT_IMMEDIATE){}

		virtual void Flush() = 0;
		virtual void GenerateMips(RHITextureRef src) = 0;
		virtual void TextureBarrier(const RHITextureBarrier& barrier) = 0;
		virtual void CopyBufferToTexture(RHIBufferRef src, uint64_t srcOffset, RHITextureRef dst, TextureSubresourceLayers dstSubresource) = 0;
		virtual void ImGuiUploadFonts() = 0;
		virtual void Destroy() override{};
	};


	
}