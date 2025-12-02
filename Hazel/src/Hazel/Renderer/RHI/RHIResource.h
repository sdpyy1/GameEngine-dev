#pragma once
#include "RHIBase.h"
#include <queue>
namespace GameEngine {
	class RHIResource
	{
	public:
		RHIResource() = delete;
		RHIResource(RHIResourceType resourceType) : resourceType(resourceType) {};
		virtual ~RHIResource() {};

		inline RHIResourceType GetType() { return resourceType; }

		virtual void* RawHandle() { return nullptr; };		// 底层资源的裸指针，仅debug时使用
		void printRawHandle();
	private:
		RHIResourceType resourceType;
		uint32_t lastUseTick = 0;

		virtual void Destroy() {};

		friend class DynamicRHI;
	};

	// ------------------------------------------------------------------------BASE ------------------------------------------------------------------------
	class RHIQueue : public RHIResource
	{
	public:
		RHIQueue(const RHIQueueInfo& info): RHIResource(RHI_QUEUE), info(info){}
		virtual void WaitIdle() = 0;

	protected:
		RHIQueueInfo info;
	};
	class RHIComputePipeline : public RHIResource
	{
	public:
		RHIComputePipeline(const RHIComputePipelineInfo& info)
			: RHIResource(RHI_COMPUTE_PIPELINE)
			, info(info)
		{
		}

	protected:
		RHIComputePipelineInfo info;
	};
	class RHISurface : public RHIResource
	{
	public:
		RHISurface() : RHIResource(RHI_SURFACE) {};

		inline Extent2D GetExetent() const { return extent; }

	protected:
		Extent2D extent;
	};

	class RHISwapchain : public RHIResource
	{
	public:
		RHISwapchain(const RHISwapchainInfo& info): RHIResource(RHI_SWAPCHAIN), info(info){}

		virtual uint32_t GetCurrentFrameIndex() = 0;
		virtual RHITextureRef GetTexture(uint32_t index) = 0;
		virtual RHITextureRef GetNewFrame(RHIFenceRef fence, RHISemaphoreRef signalSemaphore) = 0;
		virtual void Present(RHISemaphoreRef waitSemaphore) = 0;

	protected:
		RHISwapchainInfo info;
	};



	class RHICommandPool : public RHIResource, public std::enable_shared_from_this<RHICommandPool>
	{
	public:
		RHICommandPool(const RHICommandPoolInfo& info): RHIResource(RHI_COMMAND_POOL), info(info){}

		RHICommandListRef CreateCommandList(bool byPass = true);

	protected:
		RHICommandPoolInfo info;

		//std::queue<RHICommandContextRef> idleContexts = {};  // 暂未运行的线程
		//std::vector<RHICommandContextRef> contexts = {};     // 所有分配的线程
		// CriticalSectionRef sync;

		// void ReturnToPool(RHICommandContextRef commandContext) { idleContexts.push(commandContext); }
		friend class RHICommandList;
	};
	class RHIBuffer : public RHIResource
	{
	public:
		RHIBuffer(const RHIBufferInfo& info)
			: RHIResource(RHI_BUFFER)
			, info(info)
		{
		}

		virtual void* Map() = 0;
		virtual void UnMap() = 0;

		inline const RHIBufferInfo& GetInfo() const { return info; }

	protected:
		RHIBufferInfo info;
	};

	// ------------------------------------------------------------------------TEXTURE ------------------------------------------------------------------------
	class RHITexture : public RHIResource
	{
	public:
		RHITexture(const RHITextureInfo& info): RHIResource(RHI_TEXTURE), info(info){}

		Extent3D MipExtent(uint32_t mipLevel);

		inline const TextureSubresourceRange& GetDefaultSubresourceRange() const { return defaultRange; }
		inline const TextureSubresourceLayers& GetDefaultSubresourceLayers() const { return defaultLayers; }

		inline const RHITextureInfo& GetInfo() const { return info; }

	protected:
		RHITextureInfo info;

		TextureSubresourceRange defaultRange = {};
		TextureSubresourceLayers defaultLayers = {};
	};

	class RHITextureView : public RHIResource
	{
	public:
		RHITextureView(const RHITextureViewInfo& info): RHIResource(RHI_TEXTURE_VIEW), info(info){}

		inline const RHITextureViewInfo& GetInfo() const { return info; }

	protected:
		RHITextureViewInfo info;
	};
	class RHISampler : public RHIResource
	{
	public:
		RHISampler(const RHISamplerInfo& info)
			: RHIResource(RHI_SAMPLER)
			, info(info)
		{
		}

		inline const RHISamplerInfo& GetInfo() const { return info; }

	protected:
		RHISamplerInfo info;
	};
	class RHIDescriptorSet : public RHIResource
	{
	public:
		RHIDescriptorSet() : RHIResource(RHI_DESCRIPTOR_SET) {}

		virtual RHIDescriptorSet& UpdateDescriptor(const RHIDescriptorUpdateInfo& descriptorUpdateInfo) = 0;

		RHIDescriptorSet& UpdateDescriptors(const std::vector<RHIDescriptorUpdateInfo>& descriptorUpdateInfos)
		{
			for (auto& info : descriptorUpdateInfos) UpdateDescriptor(info);
			return *this;
		};
	};
	class RHIRootSignature : public RHIResource	//对pipelinelayout, descriptorSetPool等的抽象
	{
	public:
		RHIRootSignature(const RHIRootSignatureInfo& info)
			: RHIResource(RHI_ROOT_SIGNATURE)
			, info(info)
		{
		}

		virtual RHIDescriptorSetRef CreateDescriptorSet(uint32_t set) = 0;

		const RHIRootSignatureInfo& GetInfo() { return info; }

	protected:
		RHIRootSignatureInfo info;
	};

	class RHIShader : public RHIResource
	{
	public:
		RHIShader(const RHIShaderInfo& info)
			: RHIResource(RHI_SHADER)
			, info(info)
		{
			frequency = info.frequency;
		}

		ShaderFrequency GetFrequency() 				const { return frequency; }
		const ShaderReflectInfo& GetReflectInfo() 	const { return reflectInfo; }
		const RHIShaderInfo& GetInfo() 				const { return info; }
	private:
		ShaderFrequency frequency;

	protected:
		RHIShaderInfo info;
		ShaderReflectInfo reflectInfo;
	};
	class RHIGraphicsPipeline : public RHIResource
	{
	public:
		RHIGraphicsPipeline(const RHIGraphicsPipelineInfo& info)
			: RHIResource(RHI_GRAPHICS_PIPELINE)
			, info(info)
		{
		}

		const RHIGraphicsPipelineInfo& GetInfo() { return info; }

	protected:
		RHIGraphicsPipelineInfo info;
	};
	class RHIRenderPass : public RHIResource	// 在vulkan里相当于renderpass和framebuffer的整体抽象
	{
	public:
		RHIRenderPass(const RHIRenderPassInfo& info)
			: RHIResource(RHI_RENDER_PASS)
			, info(info)
		{
		}

		const RHIRenderPassInfo& GetInfo() { return info; }

	protected:
		RHIRenderPassInfo info;
	};
	// ------------------------------------------------------------------------ RayTracing ------------------------------------------------------------------------
	class RHITopLevelAccelerationStructure : public RHIResource
	{
	public:
		RHITopLevelAccelerationStructure(const RHITopLevelAccelerationStructureInfo& info)
			: RHIResource(RHI_TOP_LEVEL_ACCELERATION_STRUCTURE)
			, info(info)
		{
		}

		virtual void Update(const std::vector<RHIAccelerationStructureInstanceInfo>& instanceInfos) = 0;

		const RHITopLevelAccelerationStructureInfo& GetInfo() const { return info; }

	protected:
		RHITopLevelAccelerationStructureInfo info;
	};

	class RHIBottomLevelAccelerationStructure : public RHIResource
	{
	public:
		RHIBottomLevelAccelerationStructure(const RHIBottomLevelAccelerationStructureInfo& info)
			: RHIResource(RHI_BOTTOM_LEVEL_ACCELERATION_STRUCTURE)
			, info(info)
		{
		}

		const RHIBottomLevelAccelerationStructureInfo& GetInfo() const { return info; }

	protected:
		RHIBottomLevelAccelerationStructureInfo info;
	};

	class RHIShaderBindingTable : public RHIResource
	{
	public:
		RHIShaderBindingTable(const RHIShaderBindingTableInfo& info)
			: RHIResource(RHI_SHADER_BINDING_TABLE)
			, info(info)
		{
		}

		const RHIShaderBindingTableInfo& GetInfo() const { return info; }

	protected:
		RHIShaderBindingTableInfo info;
	};
	class RHIRayTracingPipeline : public RHIResource
	{
	public:
		RHIRayTracingPipeline(const RHIRayTracingPipelineInfo& info)
			: RHIResource(RHI_RAY_TRACING_PIPELINE)
			, info(info)
		{
		}

	protected:
		RHIRayTracingPipelineInfo info;
	};







	// ------------------------------------------------------------------------ 同步 ------------------------------------------------------------------------
	class RHIFence : public RHIResource
	{
	public:
		RHIFence(): RHIResource(RHI_FENCE){}

		virtual void Wait() = 0;
	};

	class RHISemaphore : public RHIResource
	{
	public:
		RHISemaphore(): RHIResource(RHI_SEMAPHORE){}
	};
}
