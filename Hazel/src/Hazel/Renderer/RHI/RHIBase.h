#pragma once
#include "Hazel/Core/Base.h"
namespace GameEngine {

#define SWAPCHAIN_COLOR_FORMAT FORMAT_R8G8B8A8_UNORM

#define MAX_QUEUE_CNT 2					//每个队列族的最大队列数目
#define MAX_SHADER_IN_OUT_VARIABLES 8	//允许着色器最大的输入和输出变量数目
#define MAX_RENDER_TARGETS 8			//允许同时绑定的最大RT数目
#define MAX_DESCRIPTOR_SETS 8			//允许绑定的最大描述符集数目
#define MAX_TIMEQUERYPOOL_SIZE 10000
#define RHI_COLOR_FROMAT FORMAT_R32G32B32A32_SFLOAT
#define RHI_DEPTH_FROMAT FORMAT_D32_SFLOAT

#define RHI_DYNAMICRHI DynamicRHI::Get()

	typedef std::shared_ptr<class DynamicRHI> DynamicRHIRef;
	typedef std::shared_ptr<class RHIResource> RHIResourceRef;
	typedef std::shared_ptr<class RHICommandContextImmediate> RHICommandContextImmediateRef;
	typedef std::shared_ptr<class RHIQueue> RHIQueueRef;
	typedef std::shared_ptr<class RHICommandListImmediate> RHICommandListImmediateRef;
	typedef std::shared_ptr<class RHISurface> RHISurfaceRef;
	typedef std::shared_ptr<class RHITexture> RHITextureRef;
	typedef std::shared_ptr<class RHIFence> RHIFenceRef;
	typedef std::shared_ptr<class RHISemaphore> RHISemaphoreRef;
	typedef std::shared_ptr<class RHICommandContext> RHICommandContextRef;
	typedef std::shared_ptr<class RHICommandPool> RHICommandPoolRef;
	typedef std::shared_ptr<class RHICommandList> RHICommandListRef;
	typedef std::shared_ptr<class RHISwapchain> RHISwapchainRef;
	typedef std::shared_ptr<class RHITextureView> RHITextureViewRef;
	typedef std::shared_ptr<class RHISampler> RHISamplerRef;
	typedef std::shared_ptr<class RHIShader> RHIShaderRef;
	typedef std::shared_ptr<class RHIBuffer> RHIBufferRef;
	typedef std::shared_ptr<class RHIRootSignature> RHIRootSignatureRef;
	typedef std::shared_ptr<class RHIDescriptorSet> RHIDescriptorSetRef;
	typedef std::shared_ptr<class RHIGraphicsPipeline> RHIGraphicsPipelineRef;
	typedef std::shared_ptr<class RHIRenderPass> RHIRenderPassRef;
	typedef std::shared_ptr<class RHIComputePipeline> RHIComputePipelineRef;
	typedef std::shared_ptr<class RHITopLevelAccelerationStructure> RHITopLevelAccelerationStructureRef;
	typedef std::shared_ptr<class RHIBottomLevelAccelerationStructure> RHIBottomLevelAccelerationStructureRef;
	typedef std::shared_ptr<class RHIRayTracingPipeline> RHIRayTracingPipelineRef;
	typedef std::shared_ptr<class RHIShaderBindingTable> RHIShaderBindingTableRef;

	enum API {
		API_Vulkan,
		OpenGL,
		DirectX,
		NoneAPI
	};
	struct RHIConfig {
		API api = NoneAPI;
		bool debug = false;
		bool enableRayTracing = false;
	};
	typedef struct Color3 {
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
	} Color3;
	enum RHIResourceType : uint32_t
	{
		RHI_BUFFER = 0,
		RHI_TEXTURE,
		RHI_TEXTURE_VIEW,
		RHI_SAMPLER,
		RHI_SHADER,

		// RayTracing
		RHI_SHADER_BINDING_TABLE,
		RHI_TOP_LEVEL_ACCELERATION_STRUCTURE,
		RHI_BOTTOM_LEVEL_ACCELERATION_STRUCTURE,

		RHI_ROOT_SIGNATURE,
		RHI_DESCRIPTOR_SET,

		RHI_RENDER_PASS,
		RHI_GRAPHICS_PIPELINE,
		RHI_COMPUTE_PIPELINE,
		RHI_RAY_TRACING_PIPELINE,

		RHI_QUEUE,
		RHI_SURFACE,
		RHI_SWAPCHAIN,
		RHI_COMMAND_POOL,

		RHI_COMMAND_CONTEXT,
		RHI_COMMAND_CONTEXT_IMMEDIATE,

		RHI_FENCE,
		RHI_SEMAPHORE,

		RHI_RESOURCE_TYPE_MAX_CNT,	//
	};
	const char* RHIResourceTypeToString(RHIResourceType type);

	enum QueueType : uint32_t
	{
		QUEUE_TYPE_GRAPHICS = 0,
		QUEUE_TYPE_COMPUTE,
		QUEUE_TYPE_TRANSFER,

		QUEUE_TYPE_MAX_ENUM,
	};
	enum RHIFormat : uint32_t
	{
		/*
			归一化：会再GPU读取时自动转到 0-1之间
			UNORM：无符号归一化，存储的是0-2^x-1的数据，读取时自动转为 0-1
			SNORM：有符号归一化，存储的是-2^(x-1)到2^(x-1)的数据，读取时自动转为 -1.0~1.0

			UINT：无符号整数，存储的是0-2^x-1的数据
			SINT: 有符号整数，存储的是-2^(x-1)到2^(x-1)的数据
			SFLOAT：有符号浮点数，直接作为浮点数
		
			SRGB：读取时自动转为线性，输入时自动进行伽马矫正
		*/
		FORMAT_UKNOWN = 0,
		// 	带伽马校正的色彩格式，存储时压缩、采样时解压缩
		FORMAT_R8_SRGB,
		FORMAT_R8G8_SRGB,
		FORMAT_R8G8B8_SRGB,
		FORMAT_R8G8B8A8_SRGB,
		FORMAT_B8G8R8A8_SRGB,
		// 存储高精度浮点数，支持小数和大范围数值
		FORMAT_R16_SFLOAT,
		FORMAT_R16G16_SFLOAT,
		FORMAT_R16G16B16_SFLOAT,
		FORMAT_R16G16B16A16_SFLOAT,
		FORMAT_R32_SFLOAT,
		FORMAT_R32G32_SFLOAT,
		FORMAT_R32G32B32_SFLOAT,
		FORMAT_R32G32B32A32_SFLOAT,
		// 无符号归一化格式（0~255 映射到 0.0~1.0），存储整数、使用时转浮点
		FORMAT_R8_UNORM,
		FORMAT_R8G8_UNORM,
		FORMAT_R8G8B8_UNORM,
		FORMAT_R8G8B8A8_UNORM,
		FORMAT_B8G8R8A8_UNORM,  // SwapChain支持这种格式
		FORMAT_R16_UNORM,
		FORMAT_R16G16_UNORM,
		FORMAT_R16G16B16_UNORM,
		FORMAT_R16G16B16A16_UNORM,
		// 有符号归一化格式（-128~127 映射到 - 1.0~1.0），支持正负值
		FORMAT_R8_SNORM,
		FORMAT_R8G8_SNORM,
		FORMAT_R8G8B8_SNORM,
		FORMAT_R8G8B8A8_SNORM,
		FORMAT_R16_SNORM,
		FORMAT_R16G16_SNORM,
		FORMAT_R16G16B16_SNORM,
		FORMAT_R16G16B16A16_SNORM,
		// 无符号整数格式（直接存储整数，不做归一化）	
		FORMAT_R8_UINT,
		FORMAT_R8G8_UINT,
		FORMAT_R8G8B8_UINT,
		FORMAT_R8G8B8A8_UINT,
		FORMAT_R16_UINT,
		FORMAT_R16G16_UINT,
		FORMAT_R16G16B16_UINT,
		FORMAT_R16G16B16A16_UINT,
		FORMAT_R32_UINT,
		FORMAT_R32G32_UINT,
		FORMAT_R32G32B32_UINT,
		FORMAT_R32G32B32A32_UINT,
		// 有符号整数格式（存储正负整数）	
		FORMAT_R8_SINT,
		FORMAT_R8G8_SINT,
		FORMAT_R8G8B8_SINT,
		FORMAT_R8G8B8A8_SINT,
		FORMAT_R16_SINT,
		FORMAT_R16G16_SINT,
		FORMAT_R16G16B16_SINT,
		FORMAT_R16G16B16A16_SINT,
		FORMAT_R32_SINT,
		FORMAT_R32G32_SINT,
		FORMAT_R32G32B32_SINT,
		FORMAT_R32G32B32A32_SINT,

		// 深度、模板
		FORMAT_D32_SFLOAT,
		FORMAT_D32_SFLOAT_S8_UINT,
		FORMAT_D24_UNORM_S8_UINT,

		FORMAT_MAX_ENUM, 	//
	};
	enum MemoryUsage : uint32_t
	{
		MEMORY_USAGE_UNKNOWN = 0,
		MEMORY_USAGE_GPU_ONLY = 1,		// 仅GPU使用，在VRAM显存上分配，不可绑定
		MEMORY_USAGE_CPU_ONLY = 2,		// HOST_VISIBLE &&  HOST_COHERENT 及时同步，不需要flush到GPU，GPU可访问但是很慢
		MEMORY_USAGE_CPU_TO_GPU = 3,	// HOST_VISIBLE CPU端uncached，用于CPU端频繁进行数据写入，GPU端对数据进行读取
		MEMORY_USAGE_GPU_TO_CPU = 4,	// HOST_VISIBLE CPU端cached，用于被GPU写入且被CPU读取

		MEMORY_USAGE_MAX_ENUM = 0x7FFFFFFF,		//
	};
	enum BufferCreationFlagBits : uint32_t
	{
		BUFFER_CREATION_NONE = 0x00000000,
		BUFFER_CREATION_PERSISTENT_MAP = 0x00000001,   // 强制持久映射，可由allocationInfo.pMappedData直接访问
		BUFFER_CREATION_FORCE_ALIGNMENT = 0x00000002,	// 使用256字节的内存对齐

		BUFFER_CREATION_MAX_ENUM = 0x7FFFFFFF,	//
	};
	typedef uint32_t BufferCreationFlags;


	// TODO: ???
	enum ResourceTypeBits : uint32_t
	{
		RESOURCE_TYPE_NONE = 0x00000000,
		RESOURCE_TYPE_SAMPLER = 0x00000001,
		RESOURCE_TYPE_TEXTURE = 0x00000002,
		RESOURCE_TYPE_RW_TEXTURE = 0x00000004,
		RESOURCE_TYPE_TEXTURE_CUBE = 0x00000008,
		RESOURCE_TYPE_RENDER_TARGET = 0x00000010,
		RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER = 0x00000020,
		RESOURCE_TYPE_BUFFER = 0x00000040,
		RESOURCE_TYPE_RW_BUFFER = 0x00000080,
		RESOURCE_TYPE_UNIFORM_BUFFER = 0x00000100,
		RESOURCE_TYPE_VERTEX_BUFFER = 0x00000200,
		RESOURCE_TYPE_INDEX_BUFFER = 0x00000400,
		RESOURCE_TYPE_INDIRECT_BUFFER = 0x00000800,
		RESOURCE_TYPE_TEXEL_BUFFER = 0x00001000,
		RESOURCE_TYPE_RW_TEXEL_BUFFER = 0x00002000,
		RESOURCE_TYPE_RAY_TRACING = 0x00004000,

		RESOURCE_TYPE_MAX_ENUM = 0x7FFFFFFF,	//
	};
	typedef uint32_t ResourceType;


	// 间接绘制用
	typedef struct RHIIndexedIndirectCommand
	{
		uint32_t    indexCount;
		uint32_t    instanceCount;
		uint32_t    firstIndex;
		int32_t     vertexOffset;
		uint32_t    firstInstance;
	} RHIIndexedIndirectCommand;


	// 间接绘制用
	typedef struct RHIIndirectCommand
	{
		uint32_t    vertexCount;
		uint32_t    instanceCount;
		uint32_t    firstVertex;
		uint32_t    firstInstance;
	} RHIIndirectCommand;






	typedef struct RHIBufferInfo
	{
		uint64_t size;

		MemoryUsage memoryUsage = MEMORY_USAGE_GPU_ONLY;
		ResourceType type = RESOURCE_TYPE_BUFFER;

		BufferCreationFlags creationFlag = BUFFER_CREATION_NONE;

	} RHIBufferInfo;
	
	enum TextureCreationFlagBits : uint32_t
	{
		TEXTURE_CREATION_NONE = 0x00000000,
		TEXTURE_CREATION_FORCE_2D = 0x00000001,
		TEXTURE_CREATION_FORCE_3D = 0x00000002,

		TEXTURE_CREATION_MAX_ENUM = 0x7FFFFFFF,	//
	};
	typedef uint32_t TextureCreationFlags;
	enum TextureViewType : uint32_t
	{
		VIEW_TYPE_UNDEFINED = 0,
		VIEW_TYPE_1D,
		VIEW_TYPE_2D,
		VIEW_TYPE_3D,
		VIEW_TYPE_CUBE,
		VIEW_TYPE_1D_ARRAY,
		VIEW_TYPE_2D_ARRAY,
		VIEW_TYPE_CUBE_ARRAY,

		VIEW_TYPE_MAX_ENUM,		//	
	};
	enum FilterType : uint32_t
	{
		FILTER_TYPE_NEAREST = 0,
		FILTER_TYPE_LINEAR,

		FILTER_TYPE_MAX_ENUM,	//
	};

	enum MipMapMode : uint32_t
	{
		MIPMAP_MODE_NEAREST = 0,
		MIPMAP_MODE_LINEAR,

		MIPMAP_MODE_MAX_ENUM_BIT,	//
	};
	enum AddressMode : uint32_t
	{
		ADDRESS_MODE_MIRROR,
		ADDRESS_MODE_REPEAT,
		ADDRESS_MODE_CLAMP_TO_EDGE,
		ADDRESS_MODE_CLAMP_TO_BORDER,

		ADDRESS_MODE_MAX_ENUM,	//
	};
	enum CompareFunction : uint32_t
	{
		COMPARE_FUNCTION_LESS = 0,
		COMPARE_FUNCTION_LESS_EQUAL,
		COMPARE_FUNCTION_GREATER,
		COMPARE_FUNCTION_GREATER_EQUAL,
		COMPARE_FUNCTION_EQUAL,
		COMPARE_FUNCTION_NOT_EQUAL,
		COMPARE_FUNCTION_NEVER,
		COMPARE_FUNCTION_ALWAYS,

		COMPARE_FUNCTION_MAX_ENUM,   //
	};
	enum SamplerReductionMode : uint32_t
	{
		SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE = 0,
		SAMPLER_REDUCTION_MODE_MIN,
		SAMPLER_REDUCTION_MODE_MAX,

		SAMPLER_REDUCTION_MODE_MAX_ENUM,   //
	};
	typedef struct RHISamplerInfo
	{
		FilterType minFilter = FILTER_TYPE_LINEAR;
		FilterType magFilter = FILTER_TYPE_LINEAR;
		MipMapMode mipmapMode = MIPMAP_MODE_LINEAR;
		AddressMode addressModeU = ADDRESS_MODE_REPEAT;
		AddressMode addressModeV = ADDRESS_MODE_REPEAT;
		AddressMode addressModeW = ADDRESS_MODE_REPEAT;
		CompareFunction compareFunction = COMPARE_FUNCTION_NEVER;
		SamplerReductionMode reductionMode = SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;

		float mipLodBias = 0.0f;
		float maxAnisotropy = 0.0f;

	} RHISamplerInfo;
	enum TextureAspectFlagBits : uint32_t
	{
		TEXTURE_ASPECT_NONE = 0x00000000,
		TEXTURE_ASPECT_COLOR = 0x00000001,
		TEXTURE_ASPECT_DEPTH = 0x00000002,
		TEXTURE_ASPECT_STENCIL = 0x00000004,

		TEXTURE_ASPECT_DEPTH_STENCIL = TEXTURE_ASPECT_DEPTH | TEXTURE_ASPECT_STENCIL,

		TEXTURE_ASPECT_MAX_ENUM = 0x7FFFFFFF,	//
	};
	typedef uint32_t TextureAspectFlags;
	typedef struct RHIQueueInfo
	{
		QueueType type;
		uint32_t index;

	} RHIQueueInfo;

	typedef struct Extent2D
	{
		uint32_t    width;
		uint32_t    height;

		friend bool operator==(const Extent2D& a, const Extent2D& b)
		{
			return 	a.width == b.width &&
				a.height == b.height;
		}

		const uint32_t MipSize() const
		{
			return (uint32_t)(std::floor(std::log2(std::max(width, height)))) + 1;
		}

	} Extent2D;
	typedef struct TextureSubresourceRange
	{
		TextureAspectFlags	  aspect = TEXTURE_ASPECT_NONE;
		uint32_t              baseMipLevel = 0;
		uint32_t              levelCount = 0;
		uint32_t              baseArrayLayer = 0;
		uint32_t              layerCount = 0;

		uint32_t			  __padding = 0;

		friend bool operator==(const TextureSubresourceRange& a, const TextureSubresourceRange& b)
		{
			return 	a.aspect == b.aspect &&
				a.baseMipLevel == b.baseMipLevel &&
				a.levelCount == b.levelCount &&
				a.baseArrayLayer == b.baseArrayLayer &&
				a.layerCount == b.layerCount;
		}

		bool IsDefault()
		{
			return 	aspect == TEXTURE_ASPECT_NONE &&
				baseMipLevel == 0 &&
				levelCount == 0 &&
				baseArrayLayer == 0 &&
				layerCount == 0;
		}

	} TextureSubresourceRange;

	typedef struct RHITextureViewInfo
	{
		RHITextureRef texture;
		RHIFormat format = FORMAT_UKNOWN;
		TextureViewType viewType = VIEW_TYPE_2D;

		TextureSubresourceRange subresource;

		friend bool operator== (const RHITextureViewInfo& a, const RHITextureViewInfo& b)
		{
			return  a.texture.get() == b.texture.get() &&
				a.format == b.format &&
				a.viewType == b.viewType &&
				a.subresource == b.subresource;
		}

	} RHITextureViewInfo;

	typedef struct Extent3D
	{
		uint32_t    width;
		uint32_t    height;
		uint32_t    depth;

		friend bool operator==(const Extent3D& a, const Extent3D& b)
		{
			return 	a.width == b.width &&
				a.height == b.height &&
				a.depth == b.depth;
		}
		Extent3D GetMipExtent(uint32_t mip) const
		{
			Extent3D mipExtent = *this;
			while (mip > 0 && (mipExtent.width > 1 || mipExtent.height > 1 || mipExtent.depth > 1))
			{
				if (mipExtent.width > 1) mipExtent.width /= 2;
				if (mipExtent.height > 1) mipExtent.height /= 2;
				if (mipExtent.depth > 1) mipExtent.depth /= 2;

				mip--;
			}

			return mipExtent;
		}
		const uint32_t MipSize() const
		{
			return (uint32_t)(std::floor(std::log2(std::max(width, std::max(height, depth))))) + 1;
		}

	} Extent3D;

	
	enum ShaderFrequencyBits : uint32_t
	{
		SHADER_FREQUENCY_COMPUTE = 0x00000001,
		SHADER_FREQUENCY_VERTEX = 0x00000002,
		SHADER_FREQUENCY_FRAGMENT = 0x00000004,
		SHADER_FREQUENCY_GEOMETRY = 0x00000008,
		SHADER_FREQUENCY_RAY_GEN = 0x00000010,
		SHADER_FREQUENCY_CLOSEST_HIT = 0x00000020,
		SHADER_FREQUENCY_RAY_MISS = 0x00000040,
		SHADER_FREQUENCY_INTERSECTION = 0x00000080,
		SHADER_FREQUENCY_ANY_HIT = 0x00000100,
		SHADER_FREQUENCY_MESH = 0x00000200,

		SHADER_FREQUENCY_GRAPHICS = SHADER_FREQUENCY_VERTEX |
		SHADER_FREQUENCY_FRAGMENT |
		SHADER_FREQUENCY_GEOMETRY ,/*|SHADER_FREQUENCY_MESH,*/

		SHADER_FREQUENCY_RAY_TRACING = SHADER_FREQUENCY_RAY_GEN |
		SHADER_FREQUENCY_CLOSEST_HIT |
		SHADER_FREQUENCY_RAY_MISS |
		SHADER_FREQUENCY_INTERSECTION |
		SHADER_FREQUENCY_ANY_HIT,
		SHADER_FREQUENCY_ALL = SHADER_FREQUENCY_GRAPHICS |
		SHADER_FREQUENCY_COMPUTE |
		SHADER_FREQUENCY_RAY_TRACING,

		SHADER_FREQUENCY_MAX_ENUM = 0x7FFFFFFF, //
	};
	typedef uint32_t ShaderFrequency;


	// TODO: 屏障用的
	enum RHIResourceState : uint32_t	
	{
		RESOURCE_STATE_UNDEFINED = 0,
		RESOURCE_STATE_COMMON,
		RESOURCE_STATE_TRANSFER_SRC,
		RESOURCE_STATE_TRANSFER_DST,
		RESOURCE_STATE_VERTEX_BUFFER,
		RESOURCE_STATE_INDEX_BUFFER,
		RESOURCE_STATE_COLOR_ATTACHMENT, // 颜色缓冲
		RESOURCE_STATE_DEPTH_STENCIL_ATTACHMENT, // 深度缓冲
		RESOURCE_STATE_UNORDERED_ACCESS,   // 支持随机读写（Storage Buffer / Storage Image）
		RESOURCE_STATE_SHADER_RESOURCE,
		RESOURCE_STATE_INDIRECT_ARGUMENT,
		RESOURCE_STATE_PRESENT,
		RESOURCE_STATE_ACCELERATION_STRUCTURE,

		RESOURCE_STATE_MAX_ENUM,	//
	};
	std::string RHIResourceStateToString(RHIResourceState state);


	enum PrimitiveType : uint32_t
	{
		PRIMITIVE_TYPE_TRIANGLE_LIST = 0,
		PRIMITIVE_TYPE_TRIANGLE_STRIP,
		PRIMITIVE_TYPE_LINE_LIST,
		PRIMITIVE_TYPE_POINT_LIST,

		PRIMITIVE_TYPE_MAX_ENUM,	//
	};
	typedef struct ShaderResourceEntry
	{
		// std::string name;

		uint32_t set = 0;
		uint32_t binding = 0;
		uint32_t size = 1;
		ShaderFrequency frequency = SHADER_FREQUENCY_ALL;

		ResourceType type = RESOURCE_TYPE_NONE;
		// TextureViewType textureViewType = VIEW_TYPE_UNDEFINED;	// 只有反射会填的信息，创建描述符绑定并不需要

		friend bool operator== (const ShaderResourceEntry& a, const ShaderResourceEntry& b)
		{
			return  a.set == b.set &&
				a.binding == b.binding &&
				a.size == b.size &&
				a.frequency == b.frequency &&
				a.type == b.type;
			// a.textureViewType == b.textureViewType;
		}

	} ShaderResourceEntry;
	typedef struct ShaderReflectInfo
	{
		std::string name;

		ShaderFrequency frequency;
		std::vector<ShaderResourceEntry> resources;
		std::unordered_set<std::string> definedSymbols;
		std::array<RHIFormat, MAX_SHADER_IN_OUT_VARIABLES> inputVariables = {};
		std::array<RHIFormat, MAX_SHADER_IN_OUT_VARIABLES> outputVariables = {};
		uint32_t localSizeX = 0;
		uint32_t localSizeY = 0;
		uint32_t localSizeZ = 0;

		bool DefinedSymbol(std::string symbol) const { return definedSymbols.find(symbol) != definedSymbols.end(); }

	} ShaderReflectInfo;

	// 只允许一层mip，多层layer的结构
	typedef struct TextureSubresourceLayers
	{
		TextureAspectFlags	  aspect = TEXTURE_ASPECT_NONE;
		uint32_t              mipLevel = 0;
		uint32_t              baseArrayLayer = 0;
		uint32_t              layerCount = 0;

		friend bool operator==(const TextureSubresourceLayers& a, const TextureSubresourceLayers& b)
		{
			return 	a.aspect == b.aspect &&
				a.mipLevel == b.mipLevel &&
				a.baseArrayLayer == b.baseArrayLayer &&
				a.layerCount == b.layerCount;
		}

		bool IsDefault()
		{
			return 	aspect == TEXTURE_ASPECT_NONE &&
				mipLevel == 0 &&
				baseArrayLayer == 0 &&
				layerCount == 0;
		}

	} TextureSubresourceLayers;

	typedef struct RHISwapchainInfo
	{
		RHISurfaceRef surface;
		RHIQueueRef presentQueue;

		uint32_t imageCount;
		Extent2D extent;
		RHIFormat format;

	} RHISwapchainInfo;

	typedef struct RHITextureInfo
	{
		RHIFormat format;
		Extent3D extent;
		uint32_t arrayLayers = 1;
		uint32_t mipLevels = 1;
		MemoryUsage memoryUsage = MEMORY_USAGE_GPU_ONLY;
		ResourceType type = RESOURCE_TYPE_TEXTURE;

		TextureCreationFlags creationFlag = TEXTURE_CREATION_NONE;    // 强制创建2D或3D
		friend bool operator== (const RHITextureInfo& a, const RHITextureInfo& b)
		{
			return  a.format == b.format &&
				a.extent == b.extent &&
				a.arrayLayers == b.arrayLayers &&
				a.mipLevels == b.mipLevels &&
				a.memoryUsage == b.memoryUsage &&
				a.type == b.type &&
				a.creationFlag == b.creationFlag;
		}

	} RHITextureInfo;

	typedef struct RHICommandPoolInfo
	{
		RHIQueueRef queue;

	} RHICommandPoolInfo;
	typedef struct CommandListInfo
	{
		RHICommandPoolRef pool;
		RHICommandContextRef context;

		bool byPass = true; 		//是否立即录制

	} CommandListInfo;
	typedef struct CommandListImmediateInfo
	{
		RHICommandContextImmediateRef context;

	} CommandListImmediateInfo;

	typedef struct RHIShaderInfo
	{
		std::string entry = "main";

		ShaderFrequency frequency;
		std::vector<uint8_t> code;
	} RHIShaderInfo;
	typedef struct PushConstantInfo
	{
		uint32_t size = 128;
		ShaderFrequency frequency;
	} PushConstantInfo;

	typedef struct RHIDescriptorUpdateInfo
	{
		uint32_t binding = 0;
		uint32_t index = 0;

		ResourceType resourceType = RESOURCE_TYPE_NONE;

		RHIBufferRef buffer;
		RHITextureViewRef textureView;
		RHISamplerRef sampler;
		// RHITopLevelAccelerationStructureRef tlas;

		uint64_t bufferOffset = 0;	// 仅buffer使用
		uint64_t bufferRange = 0;

	} RHIDescriptorUpdateInfo;
	typedef struct RHIRootSignatureInfo
	{
		RHIRootSignatureInfo& AddPushConstant(const PushConstantInfo& pushConstant) { pushConstants.push_back(pushConstant); return *this; }
		RHIRootSignatureInfo& AddEntry(const ShaderResourceEntry& entry);
		RHIRootSignatureInfo& AddEntry(const RHIRootSignatureInfo& other);
		RHIRootSignatureInfo& AddEntryFromReflect(RHIShaderRef shader);
		const std::vector<PushConstantInfo>& GetPushConstants() const { return pushConstants; }
		const std::vector<ShaderResourceEntry>& GetEntries() const { return entries; }

	protected:
		std::vector<ShaderResourceEntry> entries;
		std::vector<PushConstantInfo> pushConstants;

	} RHIRootSignatureInfo;


	typedef struct RHITextureBarrier
	{
		RHITextureRef texture;
		RHIResourceState srcState;
		RHIResourceState dstState;

		TextureSubresourceRange subresource = {};	// 此时取texture的默认range

	} RHITextureBarrier;



	static bool IsDepthStencilFormat(RHIFormat format)
	{
		switch (format) {
		case FORMAT_D32_SFLOAT_S8_UINT:
		case FORMAT_D24_UNORM_S8_UINT:
			return true;
		default:
			return false;
		}
	}

	static bool IsDepthFormat(RHIFormat format)
	{
		switch (format) {
		case FORMAT_D32_SFLOAT:
		case FORMAT_D32_SFLOAT_S8_UINT:
		case FORMAT_D24_UNORM_S8_UINT:
			return true;
		default:
			return false;
		}
	}

	static bool IsStencilFormat(RHIFormat format)
	{
		switch (format) {
		case FORMAT_D32_SFLOAT_S8_UINT:
		case FORMAT_D24_UNORM_S8_UINT:
			return true;
		default:
			return false;
		}
	}

	static bool IsColorFormat(RHIFormat format)
	{
		return !IsDepthFormat(format) && !IsStencilFormat(format);
	}

	static bool IsRWFormat(RHIFormat format)
	{
		switch (format) {
		case FORMAT_D32_SFLOAT:
		case FORMAT_D32_SFLOAT_S8_UINT:
		case FORMAT_D24_UNORM_S8_UINT:
		case FORMAT_R8_SRGB:
		case FORMAT_R8G8_SRGB:
		case FORMAT_R8G8B8_SRGB:
		case FORMAT_R8G8B8A8_SRGB:
		case FORMAT_B8G8R8A8_SRGB:
			return false;
		default:
			return true;
		}
	}
	typedef struct VertexElement
	{
		uint32_t streamIndex = 0;
		uint32_t attributeIndex = 0;
		RHIFormat format = FORMAT_UKNOWN;
		uint32_t offset = 0;
		uint32_t stride = 0;
		bool useInstanceIndex = false;

		bool __padding[3] = { 0 };

		friend bool operator== (const VertexElement& a, const VertexElement& b)
		{
			return  a.streamIndex == b.streamIndex &&
				a.attributeIndex == b.attributeIndex &&
				a.format == b.format &&
				a.offset == b.offset &&
				a.stride == b.stride &&
				a.useInstanceIndex == b.useInstanceIndex;
		}

	} VertexElement;
	typedef struct VertexInputStateInfo
	{
		std::vector<VertexElement> vertexElements;

		friend bool operator== (const VertexInputStateInfo& a, const VertexInputStateInfo& b)
		{
			if (a.vertexElements.size() != b.vertexElements.size()) return false;
			for (uint32_t i = 0; i < a.vertexElements.size(); i++)
			{
				if (!(a.vertexElements[i] == b.vertexElements[i])) return false;
			}
			return true;
		}

	} VertexInputStateInfo;
	enum RasterizerFillMode : uint32_t
	{
		FILL_MODE_POINT = 0,
		FILL_MODE_WIREFRAME,
		FILL_MODE_SOLID,

		FILL_MODE_MAX_ENUM,  //
	};
	enum RasterizerCullMode : uint32_t
	{
		CULL_MODE_NONE = 0,
		CULL_MODE_FRONT,
		CULL_MODE_BACK,

		CULL_MODE_MAX_ENUM,  //
	};

	enum RasterizerDepthClipMode : uint32_t
	{
		DEPTH_CLIP = 0,
		DEPTH_CLAMP,

		DEPTH_CLIP_MODE_MAX_ENUM,    //
	};
	typedef struct RHIRasterizerStateInfo
	{
		RasterizerFillMode fillMode = FILL_MODE_SOLID;
		RasterizerCullMode cullMode = CULL_MODE_BACK;
		RasterizerDepthClipMode depthClipMode = DEPTH_CLIP;

		float depthBias = 0.0f;
		float slopeScaleDepthBias = 0.0f;

		friend bool operator== (const RHIRasterizerStateInfo& a, const RHIRasterizerStateInfo& b)
		{
			return 	a.fillMode == b.fillMode &&
				a.cullMode == b.cullMode &&
				a.depthClipMode == b.depthClipMode &&
				a.depthBias == b.depthBias &&
				a.slopeScaleDepthBias == b.slopeScaleDepthBias;
		};

	} RHIRasterizerStateInfo;


	enum BlendOp : uint32_t
	{
		BLEND_OP_ADD = 0,
		BLEND_OP_SUBTRACT,
		BLEND_OP_REVERSE_SUBTRACT,
		BLEND_OP_MIN,
		BLEND_OP_MAX,

		BLEND_OP_MAX_ENUM, //
	};
	enum BlendFactor : uint32_t
	{
		BLEND_FACTOR_ZERO = 0,
		BLEND_FACTOR_ONE,
		BLEND_FACTOR_SRC_COLOR,
		BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		BLEND_FACTOR_DST_COLOR,
		BLEND_FACTOR_ONE_MINUS_DST_COLOR,
		BLEND_FACTOR_SRC_ALPHA,
		BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		BLEND_FACTOR_DST_ALPHA,
		BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
		BLEND_FACTOR_SRC_ALPHA_SATURATE,
		BLEND_FACTOR_CONSTANT_COLOR,
		BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,

		BLEND_FACTOR_MAX_ENUM, //
	};
	enum ColorWriteMaskBits : uint32_t
	{
		COLOR_MASK_RED = 0x01,
		COLOR_MASK_GREEN = 0x02,
		COLOR_MASK_BLUE = 0x04,
		COLOR_MASK_ALPHA = 0x08,

		COLOR_MASK_NONE = 0,
		COLOR_MASK_RGB = COLOR_MASK_RED | COLOR_MASK_GREEN | COLOR_MASK_BLUE,
		COLOR_MASK_RGBA = COLOR_MASK_RED | COLOR_MASK_GREEN | COLOR_MASK_BLUE | COLOR_MASK_ALPHA,
		COLOR_MASK_RG = COLOR_MASK_RED | COLOR_MASK_GREEN,
		COLOR_MASK_BA = COLOR_MASK_BLUE | COLOR_MASK_ALPHA,
	};
	typedef uint32_t ColorWriteMasks;
	typedef struct RHIBlendStateInfo
	{
		struct RenderTarget
		{
			BlendOp colorBlendOp = BLEND_OP_ADD;
			BlendFactor colorSrcBlend = BLEND_FACTOR_SRC_ALPHA;
			BlendFactor colorDstBlend = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

			BlendOp alphaBlendOp = BLEND_OP_ADD;
			BlendFactor alphaSrcBlend = BLEND_FACTOR_SRC_ALPHA;
			BlendFactor alphaDstBlend = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

			ColorWriteMasks colorWriteMask = COLOR_MASK_RGBA;

			bool enable = true;

			bool __padding[3] = { 0 };

			friend bool operator== (const RenderTarget& a, const RenderTarget& b)
			{
				return  a.colorBlendOp == b.colorBlendOp &&
					a.colorSrcBlend == b.colorSrcBlend &&
					a.colorDstBlend == b.colorDstBlend &&
					a.alphaBlendOp == b.alphaBlendOp &&
					a.alphaSrcBlend == b.alphaSrcBlend &&
					a.alphaDstBlend == b.alphaDstBlend &&
					a.colorWriteMask == b.colorWriteMask &&
					a.enable == b.enable;
			}
		};

		std::array<RenderTarget, MAX_RENDER_TARGETS> renderTargets;

		friend bool operator== (const RHIBlendStateInfo& a, const RHIBlendStateInfo& b)
		{
			return a.renderTargets == b.renderTargets;
		}

	} RHIBlendStateInfo;
	typedef struct RHIDepthStencilStateInfo
	{
		CompareFunction depthTest = COMPARE_FUNCTION_LESS_EQUAL;
		bool enableDepthTest = false;
		bool enableDepthWrite = false;

		bool __padding[2] = { 0 };

		// bool enableFrontFaceStencil = false;
		// CompareFunction frontFaceStencilTest = COMPARE_FUNCTION_ALWAYS;
		// StencilOp frontFaceStencilFailStencilOp = STENCIL_OP_KEEP;
		// StencilOp frontFaceDepthFailStencilOp = STENCIL_OP_KEEP;
		// StencilOp frontFacePassStencilOp = STENCIL_OP_KEEP;

		// bool enableBackFaceStencil = false;
		// CompareFunction backFaceStencilTest = COMPARE_FUNCTION_ALWAYS;
		// StencilOp backFaceStencilFailStencilOp = STENCIL_OP_KEEP;
		// StencilOp backFaceDepthFailStencilOp = STENCIL_OP_KEEP;
		// StencilOp backFacePassStencilOp = STENCIL_OP_KEEP;

		// uint8_t stencilReadMask = 0xFF;
		// uint8_t stencilWriteMask = 0xFF;

		friend bool operator== (const RHIDepthStencilStateInfo& a, const RHIDepthStencilStateInfo& b)
		{
			return 	a.depthTest == b.depthTest &&
				a.enableDepthTest == b.enableDepthTest &&
				a.enableDepthWrite == b.enableDepthWrite;
			// a.enableFrontFaceStencil 		== b.enableFrontFaceStencil &&
			// a.frontFaceStencilTest 			== b.frontFaceStencilTest &&
			// a.frontFaceStencilFailStencilOp == b.frontFaceStencilFailStencilOp &&
			// a.frontFaceDepthFailStencilOp 	== b.frontFaceDepthFailStencilOp &&
			// a.frontFacePassStencilOp 		== b.frontFacePassStencilOp &&
			// a.enableBackFaceStencil 		== b.enableBackFaceStencil &&
			// a.backFaceStencilTest 			== b.backFaceStencilTest &&
			// a.backFaceStencilFailStencilOp 	== b.backFaceStencilFailStencilOp &&
			// a.backFaceDepthFailStencilOp 	== b.backFaceDepthFailStencilOp &&
			// a.backFacePassStencilOp 		== b.backFacePassStencilOp &&
			// a.stencilReadMask 				== b.stencilReadMask &&
			// a.stencilWriteMask 				== b.stencilWriteMask;
		}

	} RHIDepthStencilStateInfo;
	typedef struct RHIGraphicsPipelineInfo
	{
		RHIShaderRef					vertexShader = nullptr;
		RHIShaderRef					geometryShader = nullptr;
		RHIShaderRef	 				fragmentShader = nullptr;

		RHIRootSignatureRef				rootSignature = nullptr;

		VertexInputStateInfo            vertexInputState = {};   // TODO：因为并没使用这个字段，会自动创建，会导致info的Hash计算错误，缓存错误
		PrimitiveType					primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
		RHIRasterizerStateInfo			rasterizerState = {};
		RHIBlendStateInfo				blendState = {};
		RHIDepthStencilStateInfo		depthStencilState = {};

		std::array<RHIFormat, MAX_RENDER_TARGETS> colorAttachmentFormats = { FORMAT_UKNOWN };
		RHIFormat						depthStencilAttachmentFormat = FORMAT_UKNOWN;

		uint32_t 						__padding = FORMAT_UKNOWN;

		// uint32_t						numSamples = 1;		// TODO
		// uint8_t						subpassIndex = 0;	// 放弃支持sub pass

		friend bool operator== (const RHIGraphicsPipelineInfo& a, const RHIGraphicsPipelineInfo& b)
		{
			return  a.vertexShader.get() == b.vertexShader.get() &&
				a.geometryShader.get() == b.geometryShader.get() &&
				a.fragmentShader.get() == b.fragmentShader.get() &&
				a.rootSignature.get() == b.rootSignature.get() &&
				// a.vertexInputState == b.vertexInputState &&
				a.primitiveType == b.primitiveType &&
				a.rasterizerState == b.rasterizerState &&
				a.blendState == b.blendState &&
				a.depthStencilState == b.depthStencilState;
		}

	} RHIGraphicsPipelineInfo;
	
	enum AttachmentLoadOp : uint32_t
	{
		ATTACHMENT_LOAD_OP_LOAD = 0,
		ATTACHMENT_LOAD_OP_CLEAR,
		ATTACHMENT_LOAD_OP_DONT_CARE,


		// TODO:Delete this
		Inherit, Clear, Load ,

		ATTACHMENT_LOAD_OP_MAX_ENUM,	//
	};
	enum AttachmentStoreOp : uint32_t
	{
		ATTACHMENT_STORE_OP_STORE = 0,
		ATTACHMENT_STORE_OP_DONT_CARE = 1,

		ATTACHMENT_STORE_OP_MAX_ENUM, 	//
	};
	typedef struct Color4
	{
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		float a = 0.0f;
	} Color4;
	struct AttachmentInfo
	{
		RHITextureViewRef	textureView = nullptr;

		AttachmentLoadOp 	loadOp = ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentStoreOp	storeOp = ATTACHMENT_STORE_OP_DONT_CARE;

		Color4				clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		float				clearDepth = 1.0f;
		uint32_t			clearStencil = 0;
	};
	typedef struct RHIRenderPassInfo
	{
		std::array<AttachmentInfo, MAX_RENDER_TARGETS> colorAttachments = {};
		AttachmentInfo depthStencilAttachment = {};

		Extent2D extent = { 0, 0 };
		uint32_t layers = 1;

	} RHIRenderPassInfo;
	typedef struct RHIBufferBarrier
	{
		RHIBufferRef buffer;
		RHIResourceState srcState;
		RHIResourceState dstState;

		uint32_t offset = 0;
		uint32_t size = 0;

	} RHIBufferBarrier;

	typedef struct Offset2D
	{
		uint32_t x;
		uint32_t y;

		friend Offset2D operator+(const Offset2D& a, const Offset2D& b)
		{
			return { a.x + b.x, a.y + b.y };
		}

		friend bool operator==(const Offset2D& a, const Offset2D& b)
		{
			return a.x == b.x && a.y == b.y;
		}

	} Offset2D;


	typedef struct Offset3D
	{
		uint32_t x;
		uint32_t y;
		uint32_t z;

		friend Offset3D operator+(const Offset3D& a, const Offset3D& b)
		{
			return { a.x + b.x, a.y + b.y, a.z + b.z }; 
		}

		friend bool operator==(const Offset3D& a, const Offset3D& b)
		{
			return a.x == b.x && a.y == b.y && a.z == b.z;
		}

	} Offset3D;
	typedef struct RHIComputePipelineInfo
	{
		RHIShaderRef 					computeShader;

		RHIRootSignatureRef				rootSignature;

		friend bool operator== (const RHIComputePipelineInfo& a, const RHIComputePipelineInfo& b)
		{
			return  a.computeShader.get() == b.computeShader.get() &&
				a.rootSignature.get() == b.rootSignature.get();
		}

	} RHIComputePipelineInfo;

	static uint32_t FormatChanelCounts(RHIFormat format)
	{
		switch (format) {
		case FORMAT_R8_SRGB:
		case FORMAT_R16_SFLOAT:
		case FORMAT_R32_SFLOAT:
		case FORMAT_R8_UNORM:
		case FORMAT_R16_UNORM:
		case FORMAT_R8_SNORM:
		case FORMAT_R16_SNORM:
		case FORMAT_R8_UINT:
		case FORMAT_R16_UINT:
		case FORMAT_R32_UINT:
		case FORMAT_R8_SINT:
		case FORMAT_R16_SINT:
		case FORMAT_R32_SINT:
		case FORMAT_D32_SFLOAT:
			return 1;

		case FORMAT_R8G8_SRGB:
		case FORMAT_R16G16_SFLOAT:
		case FORMAT_R32G32_SFLOAT:
		case FORMAT_R8G8_UNORM:
		case FORMAT_R16G16_UNORM:
		case FORMAT_R8G8_SNORM:
		case FORMAT_R16G16_SNORM:
		case FORMAT_R8G8_UINT:
		case FORMAT_R16G16_UINT:
		case FORMAT_R32G32_UINT:
		case FORMAT_R8G8_SINT:
		case FORMAT_R16G16_SINT:
		case FORMAT_R32G32_SINT:
		case FORMAT_D32_SFLOAT_S8_UINT:
		case FORMAT_D24_UNORM_S8_UINT:
			return 2;

		case FORMAT_R8G8B8_SRGB:
		case FORMAT_R16G16B16_SFLOAT:
		case FORMAT_R32G32B32_SFLOAT:
		case FORMAT_R8G8B8_UNORM:
		case FORMAT_R16G16B16_UNORM:
		case FORMAT_R8G8B8_SNORM:
		case FORMAT_R16G16B16_SNORM:
		case FORMAT_R8G8B8_UINT:
		case FORMAT_R16G16B16_UINT:
		case FORMAT_R32G32B32_UINT:
		case FORMAT_R8G8B8_SINT:
		case FORMAT_R16G16B16_SINT:
		case FORMAT_R32G32B32_SINT:
			return 3;

		case FORMAT_R8G8B8A8_SRGB:
		case FORMAT_B8G8R8A8_SRGB:
		case FORMAT_R16G16B16A16_SFLOAT:
		case FORMAT_R32G32B32A32_SFLOAT:
		case FORMAT_R8G8B8A8_UNORM:
		case FORMAT_R16G16B16A16_UNORM:
		case FORMAT_R8G8B8A8_SNORM:
		case FORMAT_R16G16B16A16_SNORM:
		case FORMAT_R8G8B8A8_UINT:
		case FORMAT_R16G16B16A16_UINT:
		case FORMAT_R32G32B32A32_UINT:
		case FORMAT_R8G8B8A8_SINT:
		case FORMAT_R16G16B16A16_SINT:
		case FORMAT_R32G32B32A32_SINT:
			return 4;

		default:
			return 0;
		}
	}
	struct RHIBottomLevelAccelerationStructureInfo {
		/*
			每个Mesh一个BLAS,BLAS创建Info需要提供Mesh的Buffer信息
		*/
		RHIBufferRef vertexBuffer;  // 其实只需要位置信息？
		RHIBufferRef indexBuffer;
		uint32_t vertexCount;
		uint32_t triangleCount;
		uint32_t indexOffset = 0;
		uint32_t vertexOffset = 0;
		uint32_t vertexStride = 0;

	};

	struct RHIAccelerationStructureInstanceInfo {
		float    	transform[3][4] = { 0.0f };

		uint32_t    instanceIndex;
		uint32_t    mask;
		uint32_t    shaderBindingTableOffset;
		RHIBottomLevelAccelerationStructureRef blas;
	};

	struct RHITopLevelAccelerationStructureInfo {
		uint32_t maxInstance;
		std::vector<RHIAccelerationStructureInstanceInfo> instanceInfos;
	};

	typedef struct RHIShaderBindingTableInfo
	{
		void AddRayGenGroup(RHIShaderRef rayGenShader) { rayGenGroups.push_back(rayGenShader); }
		void AddHitGroup(RHIShaderRef closestHitShader,
			RHIShaderRef anyHitShader,
			RHIShaderRef intersectionShader)
		{
			hitGroups.push_back({ closestHitShader, anyHitShader, intersectionShader });
		}
		void AddMissGroup(RHIShaderRef rayMissShader) { missGroups.push_back(rayMissShader); }

		struct HitGroup
		{
			RHIShaderRef closestHitShader;
			RHIShaderRef anyHitShader;
			RHIShaderRef intersectionShader;
		};

		std::vector<RHIShaderRef> rayGenGroups;
		std::vector<HitGroup> hitGroups;
		std::vector<RHIShaderRef> missGroups;

	} RHIShaderBindingTableInfo;

	typedef struct RHIRayTracingPipelineInfo
	{
		RHIShaderBindingTableRef 		shaderBindingTable;

		RHIRootSignatureRef				rootSignature;

		friend bool operator== (const RHIRayTracingPipelineInfo& a, const RHIRayTracingPipelineInfo& b)
		{
			return  a.shaderBindingTable.get() == b.shaderBindingTable.get() &&
				a.rootSignature.get() == b.rootSignature.get();
		}

	} RHIRayTracingPipelineInfo;
	// pass执行耗时记录
	struct RHIGPUTimeInfo{
		std::string Name;
		uint32_t StartQueryIndex;
		uint32_t EndQueryIndex;
		float DurationMs;
	};
}