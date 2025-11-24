#pragma once
#include "Volk/volk.h"
#include "Hazel/Renderer/RHI/RHI.h"
#include "Hazel/Renderer/RHI/RHICommandList.h"
#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include "VulkanRHIResource.h"
#include "VulkanRHICache.h"
namespace GameEngine
{
	class VulkanDynamicRHI : public DynamicRHI
	{
	public:
		VulkanDynamicRHI() = delete;
		VulkanDynamicRHI(const RHIConfig& config);
		virtual RHIDescriptorSetRef GetImGuiTextId(RHITextureViewRef textureView) override final;

		virtual RHISurfaceRef CreateSurface(GLFWwindow* window) override final;
		virtual RHISwapchainRef CreateSwapChain(const RHISwapchainInfo& info) override final;
		virtual RHICommandPoolRef CreateCommandPool(const RHICommandPoolInfo& info) override final;
		virtual RHICommandContextRef CreateCommandContext(RHICommandPoolRef pool) override final;

		virtual RHIQueueRef GetQueue(const RHIQueueInfo& info) override final;
		virtual void InitImGui(GLFWwindow* window) override final;

		virtual RHITextureRef CreateTexture(const RHITextureInfo& info) override final;
		virtual RHITextureViewRef CreateTextureView(const RHITextureViewInfo& info) override final;
		virtual RHIBufferRef CreateBuffer(const RHIBufferInfo& info) override final;
		virtual RHIRootSignatureRef CreateRootSignature(const RHIRootSignatureInfo& info) override final;
		virtual RHIGraphicsPipelineRef CreateGraphicsPipeline(const RHIGraphicsPipelineInfo& info) override final;
		virtual RHIRenderPassRef CreateRenderPass(const RHIRenderPassInfo& info) override final;

		virtual RHISamplerRef CreateSampler(const RHISamplerInfo& info) override final;
		virtual RHIShaderRef CreateShader(const RHIShaderInfo& info) override final;
		virtual RHIFenceRef CreateFence(bool signaled = false) override final;
		virtual RHISemaphoreRef CreateSemaphore() override final;
		virtual RHICommandListImmediateRef GetImmediateCommandList(bool start) override final;

	public:
		inline VkInstance GetInstance() const { return m_Instance; }
		inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		inline VkDevice GetDevice() const { return m_LogicalDevice; }
		inline VmaAllocator GetVMA() const { return m_MemoryAllocator; }
		inline VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
		VkRenderPass FindOrCreateVkRenderPass(const VulkanUtil::VulkanRenderPassAttachments& info) { return renderPassPool.Allocate(info).pass; }
		VkRenderPass CreateVkRenderPass(const VulkanUtil::VulkanRenderPassAttachments& info);

		VkFramebuffer FindOrCreateVkFramebuffer(const VkFramebufferCreateInfo& info) { return frameBufferPool.Allocate(info).frameBuffer; }
		VkFramebuffer CreateVkFramebuffer(const VkFramebufferCreateInfo& info);
	private:

		void CreateInstance();
		void CreatePhysicalDevice();
		void CreateLogicalDevice();
		void CreateQueues();
		void CreateMemoryAllocator();
		void CreateDescriptorPool();
		void CreateImmediateCommand();
	private:
		// 实例
		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		std::vector<VkLayerProperties> m_AvailableLayers;

		// 物理设备
		VkPhysicalDevice m_PhysicalDevice;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
		VkPhysicalDeviceFeatures m_PhysicalDeviceFeatures;
		VkPhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;
		VkPhysicalDeviceSamplerFilterMinmaxProperties filterMinmaxProperties;           //采样器特性
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_PhysicalDeviceRayTracingPipelineProperties;   //光追特性
		std::vector<std::string> m_PhysicalDeviceSupportedExtensions;
		// 逻辑设备
		VkDevice m_LogicalDevice = nullptr;

		// 队列
		std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties; // 所有队列族
		std::array<int32_t, QUEUE_TYPE_MAX_ENUM> m_QueueIndices; // 每种类型的队列来自哪一个队列族
		std::array<std::array<RHIQueueRef, MAX_QUEUE_CNT>, QUEUE_TYPE_MAX_ENUM> m_Queues; // 外层是类型索引，每个类型有多个Queue，由QUEUE_TYPE_MAX_ENUM控制

		// VMA
		VmaAllocator m_MemoryAllocator;

		// 描述符
		VkDescriptorPool m_DescriptorPool;

		// 立即模式命令队列
		RHICommandContextImmediateRef m_ImmediateCommandContext;
		RHICommandListImmediateRef m_ImmediateCommandList;

		// 池化的renderPass和frameBuffer
		VkRenderPassCache renderPassPool;
		VkFramebufferCache frameBufferPool;
	};


	class VulkanRHICommandContext : public RHICommandContext {
	public:
		VulkanRHICommandContext(RHICommandPoolRef pool);
		virtual void BeginCommand() override final;
		virtual void TextureBarrier(const RHITextureBarrier& barrier) override final;

		virtual void BufferBarrier(const RHIBufferBarrier& barrier) override final;
		virtual void BeginRenderPass(RHIRenderPassRef renderPass) override final;
		virtual void CopyTexture(RHITextureRef src, TextureSubresourceLayers srcSubresource, RHITextureRef dst, TextureSubresourceLayers dstSubresource) override final;
		virtual void PushLabel(const std::string& name, Color3 color = { 0.0f, 0.0f, 0.0f }) override final;

		virtual void PopLabel() override final;
		virtual void GenerateMips(RHITextureRef src) override final;
		virtual void EndRenderPass() override final;
		virtual void EndCommand() override final;
		virtual void Execute(RHIFenceRef fence, RHISemaphoreRef waitSemaphore, RHISemaphoreRef signalSemaphore) override final;
		virtual void SetViewport(Offset2D min, Offset2D max) override final;
		virtual void SetScissor(Offset2D min, Offset2D max) override final;
		virtual void SetDepthBias(float constantBias, float slopeBias, float clampBias) override final;
		virtual void SetLineWidth(float width) override final;
		virtual void SetGraphicsPipeline(RHIGraphicsPipelineRef graphicsPipeline) override final;
		virtual void SetComputePipeline(RHIComputePipelineRef computePipeline) override final;
		// virtual void SetRayTracingPipeline(RHIRayTracingPipelineRef rayTracingPipeline) override final;
		virtual void PushConstants(void* data, uint16_t size, ShaderFrequency frequency) override final;
		virtual void BindDescriptorSet(RHIDescriptorSetRef descriptor, uint32_t set) override final;
		virtual void BindVertexBuffer(RHIBufferRef vertexBuffer, uint32_t streamIndex, uint32_t offset) override final;
		virtual void BindIndexBuffer(RHIBufferRef indexBuffer, uint32_t offset) override final;
		virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override final;
		virtual void DispatchIndirect(RHIBufferRef argumentBuffer, uint32_t argumentOffset) override final;
		virtual void TraceRays(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override final;
		virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override final;
		virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) override final;
		virtual void DrawIndirect(RHIBufferRef argumentBuffer, uint32_t offset, uint32_t drawCount) override final;
		virtual void DrawIndexedIndirect(RHIBufferRef argumentBuffer, uint32_t offset, uint32_t drawCount) override final;
		virtual void ImGuiRenderDrawData() override final;
		virtual std::vector<RHIGPUTimeInfo> GetGPUTime() override final;

		virtual void* RawHandle() override final { return handle; }

	private:
		VkCommandBuffer handle;
		std::shared_ptr<VulkanRHICommandPool> pool;
		VulkanRHIRenderPass* renderPass;                // 运行时状态，随指令变化
		VulkanRHIGraphicsPipeline* graphicsPipeline;
		VulkanRHIComputePipeline* computePipeline;
		VkQueryPool m_TimestampQueryPool;
		uint32_t m_TimestampQueryIndex = 0;
		// 用于在渲染时临时存储标签信息的栈
		std::vector<RHIGPUTimeInfo> m_ActiveLabels;
		// 用于存储最终计算好的耗时结果
		std::vector<RHIGPUTimeInfo> m_FinishedLabels;
		VkPipelineLayout GetCuttentPipelineLayout();
		VkPipelineBindPoint GetCuttentBindingPoint();
		void ResetQueryState();
	};



	class VulkanRHICommandContextImmediate : public RHICommandContextImmediate
	{
	public:
		VulkanRHICommandContextImmediate();

		virtual void Flush() override final;
		virtual void GenerateMips(RHITextureRef src) override final;
		virtual void ImGuiUploadFonts() override final;
		virtual void CopyBufferToTexture(RHIBufferRef src, uint64_t srcOffset, RHITextureRef dst, TextureSubresourceLayers dstSubresource) override final;

		virtual void TextureBarrier(const RHITextureBarrier& barrier) override final;


	private:
		void BeginSingleTimeCommand();
		void EndSingleTimeCommand();
		RHIFenceRef fence;
		RHIQueueRef queue;
		RHICommandPoolRef commandPool;
		VkCommandBuffer oldHandle = VK_NULL_HANDLE;

		VkCommandBuffer handle;
		friend class VulkanDynamicRHI;
	};
}
