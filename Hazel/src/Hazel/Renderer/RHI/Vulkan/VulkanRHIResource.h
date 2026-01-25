#pragma once
#include "Volk/volk.h"
#include "Hazel/Renderer/RHI/RHIResource.h"
#include <GLFW/glfw3.h>
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

namespace GameEngine {
	class VulkanDynamicRHI;
	class VulkanRHIQueue : public RHIQueue
	{
	public:
		VulkanRHIQueue(const RHIQueueInfo& info, VkQueue queue, uint32_t queueFamilyIndex): RHIQueue(info), handle(queue), queueFamilyIndex(queueFamilyIndex){}
		virtual void WaitIdle() override final { vkQueueWaitIdle(handle); }
		virtual void* RawHandle() override final { return handle; };
		const VkQueue& GetHandle() { return handle; }
		uint32_t GetQueueFamilyIndex() { return queueFamilyIndex; }
		virtual void Destroy() override final {};

	private:
		VkQueue handle;
		uint32_t queueFamilyIndex;
	};
	class VulkanRHISurface : public RHISurface
	{
	public:
		VulkanRHISurface(GLFWwindow* window);

		const VkSurfaceKHR& GetHandle() { return handle; }

		virtual void Destroy() override final;

	private:
		VkSurfaceKHR handle;
	};
	class VulkanRHIComputePipeline : public RHIComputePipeline
	{
	public:
		VulkanRHIComputePipeline(const RHIComputePipelineInfo& info);

		VkPipelineLayout GetPipelineLayout() { return pipelineLayout; }

		const VkPipeline& GetHandle() { return handle; }

		void Bind(VkCommandBuffer commandBuffer);

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkPipeline handle;
		VkPipelineLayout pipelineLayout;
	};
	class VulkanRHISwapchain : public RHISwapchain
	{
	public:
		VulkanRHISwapchain(const RHISwapchainInfo& info);

		virtual uint32_t GetCurrentFrameIndex() override final { return currentIndex; }
		virtual RHITextureRef GetTexture(uint32_t index) override final { return textures[index]; }
		virtual RHITextureRef GetNewFrame(RHIFenceRef fence, RHISemaphoreRef signalSemaphore) override final;
		virtual void Present(RHISemaphoreRef waitSemaphore) override final;

		const VkSwapchainKHR& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkSwapchainKHR handle;
		VkSurfaceFormatKHR surfaceFormat;
		VkPresentModeKHR presentMode;

		std::vector<VkImage> images;
		VkFormat imageFormat;
		VkExtent2D imageExtent;

		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> availableFormats;
		std::vector<VkPresentModeKHR> availablePresentModes;

		std::vector<RHITextureRef> textures;
		uint32_t currentIndex;

		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(VkFormat targetFormat);
		VkPresentModeKHR ChooseSwapPresentMode();
		VkExtent2D ChooseSwapExtent();
		void Resize();
	};
	class VulkanRHITexture : public RHITexture
	{
	public:
		VulkanRHITexture(const RHITextureInfo& info, VkImage image = VK_NULL_HANDLE);

		const VkImage& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkImage handle;

		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;
	};
	class VulkanRHITextureView : public RHITextureView
	{
	public:
		VulkanRHITextureView(const RHITextureViewInfo& info);

		const VkImageView& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkImageView handle;
	};
	class VulkanRHICommandPool : public RHICommandPool
	{
	public:
		VulkanRHICommandPool(const RHICommandPoolInfo& info);

		RHIQueueRef GetQueue() { return info.queue; }

		const VkCommandPool& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkCommandPool handle;
	};

	class VulkanRHISampler : public RHISampler
	{
	public:
		VulkanRHISampler(const RHISamplerInfo& info);

		const VkSampler& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkSampler handle;
	};
	class VulkanRHIShader : public RHIShader
	{
	public:
		VulkanRHIShader(const RHIShaderInfo& info);

		VkPipelineShaderStageCreateInfo GetShaderStageCreateInfo();

		const VkShaderModule& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkShaderModule handle;
	};
	class VulkanRHIDescriptorSet : public RHIDescriptorSet
	{
	public:
		VulkanRHIDescriptorSet(VkDescriptorSetLayout setLayout);
		VulkanRHIDescriptorSet(VkDescriptorSet aSet);   // ImGUI专用

		virtual RHIDescriptorSet& UpdateDescriptor(const RHIDescriptorUpdateInfo& descriptorUpdateInfo) override final;

		const VkDescriptorSet& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkDescriptorSet handle;
		friend class VulkanDynamicRHI;

	};
	class VulkanRHIRenderPass : public RHIRenderPass
	{
	public:
		VulkanRHIRenderPass(const RHIRenderPassInfo& info);

		const VkRenderPass& GetHandle() { return handle; }
		const VkFramebuffer& GetFrameBuffer() { return frameBuffer; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkRenderPass handle;
		VkFramebuffer frameBuffer;
	};
	class VulkanRHIGraphicsPipeline : public RHIGraphicsPipeline
	{
	public:
		VulkanRHIGraphicsPipeline(const RHIGraphicsPipelineInfo& info);

		VkPipelineLayout GetPipelineLayout() { return pipelineLayout; }

		const VkPipeline& GetHandle() { return handle; }

		void Bind(VkCommandBuffer commandBuffer);

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkPipeline handle;
		VkPipelineLayout pipelineLayout;

		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		std::vector<VkPipelineColorBlendAttachmentState> blendStates;

		// 默认开启的动态设置状态
		std::vector<VkDynamicState> dynamicStates =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_LINE_WIDTH,
			VK_DYNAMIC_STATE_VERTEX_INPUT_EXT,
			VK_DYNAMIC_STATE_DEPTH_BIAS,
			// VK_DYNAMIC_STATE_BLEND_CONSTANTS,
			// VK_DYNAMIC_STATE_DEPTH_BOUNDS,
			// VK_DYNAMIC_STATE_STENCIL_REFERENCE
		};

		VkPipelineVertexInputStateCreateInfo GetInputStateCreateInfo(const VertexInputStateInfo& vertexInputState);
		VkPipelineInputAssemblyStateCreateInfo GetPipelineInputAssemblyStateCreateInfo(const PrimitiveType& primitiveType);
		VkPipelineViewportStateCreateInfo GetPipelineViewportStateCreateInfo();
		VkPipelineRasterizationStateCreateInfo GetPipelineRasterizationStateCreateInfo(const RHIRasterizerStateInfo& rasterizerState);
		VkPipelineMultisampleStateCreateInfo GetPipelineMultisampleStateCreateInfo();
		VkPipelineColorBlendStateCreateInfo GetPipelineColorBlendStateCreateInfo(const RHIBlendStateInfo& blendState, uint32_t size);
		VkPipelineDepthStencilStateCreateInfo GetPipelineDepthStencilStateCreateInfo(const RHIDepthStencilStateInfo& depthStencilState);
		VkPipelineDynamicStateCreateInfo GetPipelineDynamicStateCreateInfo();

		void GetDynamicInputStateCreateInfo(const VertexInputStateInfo& vertexInputState);
		std::vector<VkVertexInputAttributeDescription2EXT> dynamicAttributeDescriptions;
		std::vector<VkVertexInputBindingDescription2EXT> dynamicBindingDescriptions;
	};

	class VulkanRHIRootSignature : public RHIRootSignature
	{
	public:
		struct SetInfo
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			VkDescriptorSetLayout layout;
		};

		VulkanRHIRootSignature(const RHIRootSignatureInfo& info);

		virtual RHIDescriptorSetRef CreateDescriptorSet(uint32_t set) override final;

		const std::vector<SetInfo>& GetSetInfos() { return setInfos; }

		virtual void Destroy() override final;

	private:
		std::vector<SetInfo> setInfos;
		friend class VulkanDynamicRHI;
		friend class DynamicRHI;
	};
	class VulkanRHIBuffer : public RHIBuffer
	{
	public:
		VulkanRHIBuffer(const RHIBufferInfo& info);

		const VkBuffer& GetHandle() { return handle; }

		virtual void* Map() override final;
		virtual void UnMap() override final;

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkBuffer handle;

		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;

		bool mapped = false;
		void* pointer = nullptr;
	};

	class VulkanRHIShaderBindingTable : public RHIShaderBindingTable
	{
	public:
		VulkanRHIShaderBindingTable(const RHIShaderBindingTableInfo& info);

		const std::vector<VkPipelineShaderStageCreateInfo>& GetStages() { return stages; }
		const std::vector<VkRayTracingShaderGroupCreateInfoKHR>& GetGroups() { return groups; }

		uint32_t GetRayGenGroupSize() { return rayGenGroupSize; }
		uint32_t GetHitGroupSize() { return hitGroupSize; }
		uint32_t GetRayMissGroupSize() { return rayMissGroupSize; }

		virtual void Destroy() override final;

	private:
		std::vector<VkPipelineShaderStageCreateInfo>        stages;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR>   groups;
		uint32_t rayGenGroupSize = 0;
		uint32_t hitGroupSize = 0;
		uint32_t rayMissGroupSize = 0;
	};
	class VulkanRHIRayTracingPipeline : public RHIRayTracingPipeline
	{
	public:
		VulkanRHIRayTracingPipeline(const RHIRayTracingPipelineInfo& info);

		VkPipelineLayout GetPipelineLayout() { return pipelineLayout; }

		const VkPipeline& GetHandle() { return handle; }
		const VkStridedDeviceAddressRegionKHR& GetRaygenRegion() { return raygenRegion; }
		const VkStridedDeviceAddressRegionKHR& GetRayMissRegion() { return missRegion; }
		const VkStridedDeviceAddressRegionKHR& GetHitRegion() { return hitRegion; }
		const VkStridedDeviceAddressRegionKHR& GetCallableRegion() { return callableRegion; }

		void Bind(VkCommandBuffer commandBuffer);

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		void BuildShaderGroupHandle();
		VkStridedDeviceAddressRegionKHR raygenRegion{};
		VkStridedDeviceAddressRegionKHR missRegion{};
		VkStridedDeviceAddressRegionKHR hitRegion{};
		VkStridedDeviceAddressRegionKHR callableRegion{};   //TODO：还未实现

		RHIBufferRef shaderGroupHandleBuffer;	// 用于存储SBT的全部句柄

		VkPipeline handle;
		VkPipelineLayout pipelineLayout;
	};

	class VulkanRHITopLevelAccelerationStructure : public RHITopLevelAccelerationStructure
	{
	public:
		VulkanRHITopLevelAccelerationStructure(const RHITopLevelAccelerationStructureInfo& info);

		const VkAccelerationStructureKHR& GetHandle() { return handle; }
		VkDeviceAddress GetAddress() { return address; }

		virtual void Update(const std::vector<RHIAccelerationStructureInstanceInfo>& instanceInfos) override final;

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
		VkDeviceAddress address;
		RHIBufferRef accelerationStructureBuffer;	// 加速结构占用的内存
		RHIBufferRef instanceBuffer;				// 实例信息内存
	};
	class VulkanRHIBottomLevelAccelerationStructure : public RHIBottomLevelAccelerationStructure
	{
	public:
		VulkanRHIBottomLevelAccelerationStructure(const RHIBottomLevelAccelerationStructureInfo& info);

		const VkAccelerationStructureKHR& GetHandle() { return handle; }
		VkDeviceAddress GetAddress() { return address; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkAccelerationStructureKHR handle;
		VkDeviceAddress address;
		RHIBufferRef accelerationStructureBuffer;
	};



	//同步 ////////////////////////////////////////////////////////////////////////////////////////////////////////

	class VulkanRHIFence : public RHIFence
	{
	public:
		VulkanRHIFence(bool signaled);
		virtual void WaitAndReset() override final;
		virtual bool GetStatus() override final;

		const VkFence& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkFence handle;
	};

	class VulkanRHISemaphore : public RHISemaphore
	{
	public:
		VulkanRHISemaphore();

		const VkSemaphore& GetHandle() { return handle; }

		virtual void Destroy() override final;
		virtual void* RawHandle() override final { return handle; };

	private:
		VkSemaphore handle;
	};

}

