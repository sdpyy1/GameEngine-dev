#include "hzpch.h"
#include "VulkanRHI.h"
#include "VulkanUtil.h"
#include "VulkanRHIResource.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include <Hazel/Core/Definations.h>

#define VMA_IMPLEMENTATION

#define VULKAN_VERSION VK_API_VERSION_1_2

namespace GameEngine
{
	VulkanDynamicRHI::VulkanDynamicRHI(const RHIConfig& config): DynamicRHI(config)
	{
		CreateInstance();
		CreatePhysicalDevice();
        CreateLogicalDevice();
        CreateQueues();
        CreateMemoryAllocator();
        CreateDescriptorPool();
	}

	void VulkanDynamicRHI::CreateInstance()
	{
		// volk
        if (volkInitialize() != VK_SUCCESS) {
            LOG_ERROR("Volk initialize failed!");
            return;
        }
		// 获取可用的实例层
		{
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
			m_AvailableLayers = std::vector<VkLayerProperties>(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, m_AvailableLayers.data());
			/*for (auto& layer : availableLayers)
			{
				LOG_TRACE("Layer: {}, Description: {}, Spec Version: {}, Implementation Version: {}",layer.layerName,layer.description,layer.specVersion,layer.implementationVersion);
			}*/
		}

		// VkApplicationInfo
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Toy Render Engine";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VULKAN_VERSION;


		// VkInstance
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto validationLayers = VulkanUtil::GetDebugMessengerCreateInfo();
		auto validationFeature = VulkanUtil::GetValidationFeatureCreateInfo();
		std::vector<const char*> layers;
		if (m_Config.debug)
		{
			for (auto& layer : INSTANCE_LAYERS) layers.push_back(layer);

			createInfo.enabledLayerCount = (uint32_t)layers.size();
			createInfo.ppEnabledLayerNames = layers.data();
			createInfo.pNext = &validationLayers;
			validationLayers.pNext = &validationFeature;
		}

		auto instanceExtentions = VulkanUtil::GetRequiredInstanceExtensions(m_Config.debug);
		createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtentions.size());
		createInfo.ppEnabledExtensionNames = instanceExtentions.data();

		// 创建Vulkan实例
        VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_Instance));
		volkLoadInstance(m_Instance);
		
		//创建DebugMessager
		if (m_Config.debug)
		{
			VkDebugUtilsMessengerCreateInfoEXT info = VulkanUtil::GetDebugMessengerCreateInfo();
			if (vkCreateDebugUtilsMessengerEXT(m_Instance, &info, nullptr, &m_DebugMessenger) != VK_SUCCESS){LOG_ERROR("Failed to set up debug messenger!");}
		}
	}

	void VulkanDynamicRHI::CreatePhysicalDevice()
	{
        // 获取可用物理设备
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

        for (auto& device : devices) {
            vkGetPhysicalDeviceProperties(device, &m_PhysicalDeviceProperties);
            // 直接找AMD或NVIDIA，找到就创建，不支持集成GPU
            for (auto target : TARGET_DEVICES)
            {
                std::string name = std::string(m_PhysicalDeviceProperties.deviceName);
                std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
                if (name.find(target) != std::string::npos)
                {
                    {
                        LOG_TRACE_TAG("PhysicalDevice", LOG_LINE);
                        LOG_TRACE_TAG("PhysicalDevice", "Device Name: {}", m_PhysicalDeviceProperties.deviceName);
                        LOG_TRACE_TAG("PhysicalDevice", "framebufferColorSampleCounts: {}", m_PhysicalDeviceProperties.limits.framebufferColorSampleCounts);
                        LOG_TRACE_TAG("PhysicalDevice", "framebufferDepthSampleCounts: {}", m_PhysicalDeviceProperties.limits.framebufferDepthSampleCounts);
                        LOG_TRACE_TAG("PhysicalDevice", "framebufferStencilSampleCounts: {}", m_PhysicalDeviceProperties.limits.framebufferStencilSampleCounts);
                        LOG_TRACE_TAG("PhysicalDevice", "maxColorAttachments: {}", m_PhysicalDeviceProperties.limits.maxColorAttachments);
                        LOG_TRACE_TAG("PhysicalDevice", "maxDescriptorSetInputAttachments: {}", m_PhysicalDeviceProperties.limits.maxDescriptorSetInputAttachments);
                        LOG_TRACE_TAG("PhysicalDevice", "maxDescriptorSetUniformBuffers: {}", m_PhysicalDeviceProperties.limits.maxDescriptorSetUniformBuffers);
                        LOG_TRACE_TAG("PhysicalDevice", "maxFramebufferLayers: {}", m_PhysicalDeviceProperties.limits.maxFramebufferLayers);
                        LOG_TRACE_TAG("PhysicalDevice", "maxPushConstantsSize: {} bytes", m_PhysicalDeviceProperties.limits.maxPushConstantsSize);
                        LOG_TRACE_TAG("PhysicalDevice", "maxBoundDescriptorSets: {}", m_PhysicalDeviceProperties.limits.maxBoundDescriptorSets);
                        LOG_TRACE_TAG("PhysicalDevice", "maxComputeWorkGroupInvocations: {}", m_PhysicalDeviceProperties.limits.maxComputeWorkGroupInvocations);
                        LOG_TRACE_TAG("PhysicalDevice", "minStorageBufferOffsetAlignment: {} bytes", m_PhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment);
                        LOG_TRACE_TAG("PhysicalDevice", LOG_LINE);
                    }

                    //初始化存储物理设备支持的各种属性，特性，扩展，内存特性，队列信息
                    //vkGetPhysicalDeviceFeatures2(device, &fetures2);
                    vkGetPhysicalDeviceFeatures(device, &m_PhysicalDeviceFeatures);
                    vkGetPhysicalDeviceMemoryProperties(device, &m_PhysicalDeviceMemoryProperties);
    
                    // 获取所有支持的拓展
                    uint32_t extCount = 0;
                    vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
                    std::vector<VkExtensionProperties> extensions(extCount);
                    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
                    {
                        for (auto& ext : extensions)
                        {
                            m_PhysicalDeviceSupportedExtensions.push_back(ext.extensionName);
                            // LOG_TRACE_TAG("PhysicalDevice", "Support Extension: {}", ext.extensionName);
                        }
                    }

                    if (m_Config.enableRayTracing)
                    {
                        //获取光追管线特性
                        m_PhysicalDeviceRayTracingPipelineProperties = {};
                        m_PhysicalDeviceRayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

                        VkPhysicalDeviceProperties2 deviceProperties2 = {};
                        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                        deviceProperties2.pNext = &m_PhysicalDeviceRayTracingPipelineProperties;
                        vkGetPhysicalDeviceProperties2(device, &deviceProperties2);
                    }

                    m_PhysicalDevice = device;
                    return;
                }
            }
        }
        LOG_ERROR("Target device not found!");
    }

    void VulkanDynamicRHI::CreateLogicalDevice() {
        // 获取队列族信息
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
        m_QueueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, m_QueueFamilyProperties.data());

        for (auto& index : m_QueueIndices) index = -1;


        /*for (auto& queueFamily : m_QueueFamilyProperties)
        {
            LOG_TRACE_TAG("Queue", "Queue Count: {}", queueFamily.queueCount);
            LOG_TRACE_TAG("Queue", "Queue Flags: {}", VulkanUtil::QueueFlagsToString(queueFamily.queueFlags));
        }*/

        // 遍历队列族，寻找需要的队列的支持，找到支持的队列组后后续族就不需要创建了，每个类型队列会创建2个Queue
        std::vector<uint32_t> allocatedCounts(m_QueueFamilyProperties.size()); // 记录每个队列组需要申请的Queue数量，提供给逻辑设备创建
        for (int i = 0; i < m_QueueFamilyProperties.size(); i++)
        {
            auto& queueFamily = m_QueueFamilyProperties[i];
            uint32_t queueCount = queueFamily.queueCount;
            if (queueCount > MAX_QUEUE_CNT && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && m_QueueIndices[QUEUE_TYPE_GRAPHICS] < 0)
            {
                m_QueueIndices[QUEUE_TYPE_GRAPHICS] = i;
                queueCount -= MAX_QUEUE_CNT;
            }
            if (queueCount > MAX_QUEUE_CNT &&queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT &&m_QueueIndices[QUEUE_TYPE_COMPUTE] < 0)
            {
                m_QueueIndices[QUEUE_TYPE_COMPUTE] = i;
                queueCount -= MAX_QUEUE_CNT;
            }
            if (queueCount > MAX_QUEUE_CNT &&queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT &&m_QueueIndices[QUEUE_TYPE_TRANSFER] < 0)
            {
                m_QueueIndices[QUEUE_TYPE_TRANSFER] = i;
                queueCount -= MAX_QUEUE_CNT;
            }

            allocatedCounts[i] = queueFamily.queueCount - queueCount;

            for (int i = 0; i < QUEUE_TYPE_MAX_ENUM; i++) if (m_QueueIndices[i] < 0) LOG_ERROR("Fail to allocate queue!");

        }

        // 创建设备请求信息
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        //队列信息 要声明每个队列组需要创建的Queue数量
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        for (int i = 0; i < allocatedCounts.size(); i++)
        {
            if (allocatedCounts[i] == 0) continue;
            std::vector<float> priorities(allocatedCounts[i], 1.0f);

            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = i;                               //队列族
            queueCreateInfo.queueCount = allocatedCounts[i];                    //队列数目，多个队列可以支持并行异步计算
            queueCreateInfo.pQueuePriorities = QUEUE_PRIORITIES;                //优先级
            queueCreateInfos.push_back(queueCreateInfo);
        }
        createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        //扩展信息
        std::vector<const char*> deviceExtentions;
        for (auto extention : DEVICE_EXTENTIONS) { 
            if (!VulkanUtil::IsExtensionSupported(m_PhysicalDeviceSupportedExtensions, extention)) continue;
            deviceExtentions.push_back(extention);
        };
        if (m_Config.enableRayTracing) { 
            for (auto extention : RAY_TRACING_DEVICE_EXTENTIONS)
                deviceExtentions.push_back(extention);  // TODO：不检查直接添加不会报错，但是RenderDoc会报错，但是检查添加了，直接运行也会报错
        }
        createInfo.enabledExtensionCount = (uint32_t)deviceExtentions.size();
        createInfo.ppEnabledExtensionNames = deviceExtentions.data();

        // 设备Layer已被废弃
        /*std::vector<const char*> devicelayers;
        if (m_Config.debug)
        {
            for (auto layer : DEVICE_LAYERS) devicelayers.push_back(layer);

            createInfo.enabledLayerCount = (uint32_t)devicelayers.size();
            createInfo.ppEnabledLayerNames = devicelayers.data();
        }*/

        // 特性支持
        VkPhysicalDeviceFeatures deviceFeatures = {};       //设备特性信息
        deviceFeatures.samplerAnisotropy = VK_TRUE;         //请求各向异性采样支持
        deviceFeatures.geometryShader = VK_TRUE;            //几何着色器支持
        deviceFeatures.tessellationShader = VK_TRUE;        //曲面细分着色器支持
        deviceFeatures.pipelineStatisticsQuery = VK_TRUE;   //管线统计查询支持
        deviceFeatures.fillModeNonSolid = VK_TRUE;          //线框模式
        deviceFeatures.multiDrawIndirect = VK_TRUE;         //多重间接绘制
        deviceFeatures.independentBlend = VK_TRUE;          //MRT单独设置每个混合状态
        deviceFeatures.drawIndirectFirstInstance = VK_TRUE; //允许间接绘制的firstInstance不为0
        deviceFeatures.shaderInt64 = VK_TRUE;               //64位支持
        deviceFeatures.shaderFloat64 = VK_TRUE;
        createInfo.pEnabledFeatures = &deviceFeatures;

        VkPhysicalDeviceVulkan12Features vulkan12Features{};                                        //1.2版本的其他支持
        vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        vulkan12Features.samplerFilterMinmax = VK_TRUE;                                             //采样器的过滤模式，用于Hiz
        //bindless支持，描述符索引
        vulkan12Features.runtimeDescriptorArray = VK_TRUE;                                          //SPIR-V 中使用动态数组
        vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;                        //DescriptorSet Layout 的 Binding 中使用可变大小的 AoD
        vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;                       //SPIR-V 中通过 nonuniformEXT Decoration 用于非 Uniform 变量下标索引资源数组
        vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
        vulkan12Features.descriptorIndexing = VK_TRUE;
        vulkan12Features.bufferDeviceAddress = VK_TRUE;
        createInfo.pNext = &vulkan12Features;

        VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT dynamicVertexInputFeatures = {};         //动态顶点输入描述
        dynamicVertexInputFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT;
        dynamicVertexInputFeatures.vertexInputDynamicState = VK_TRUE;
        vulkan12Features.pNext = &dynamicVertexInputFeatures;

        if (m_Config.enableRayTracing)
        {
            VkPhysicalDeviceBufferDeviceAddressFeaturesEXT deviceAddressfeture = {};                //请求内存地址信息
            deviceAddressfeture.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;
            deviceAddressfeture.bufferDeviceAddress = VK_TRUE;

            VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};    //rt加速结构
            accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            accelerationStructureFeatures.accelerationStructure = true;

            VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {};          //rt管线
            rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            rayTracingPipelineFeatures.rayTracingPipeline = true;

            VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {};                              //rt查询
            rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
            rayQueryFeatures.rayQuery = VK_TRUE;
            dynamicVertexInputFeatures.pNext = &deviceAddressfeture;
            deviceAddressfeture.pNext = &accelerationStructureFeatures;
            accelerationStructureFeatures.pNext = &rayTracingPipelineFeatures;
            rayTracingPipelineFeatures.pNext = &rayQueryFeatures;
        }
        VK_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice));
        volkLoadDevice(m_LogicalDevice);  //volk
    }

    void VulkanDynamicRHI::CreateQueues()
    {
        std::vector<uint32_t> offsets(m_QueueFamilyProperties.size(), { 0 });
        for (uint32_t i = 0; i < QUEUE_TYPE_MAX_ENUM; i++)
        {
            for (uint32_t j = 0; j < MAX_QUEUE_CNT; j++)
            {
                VkQueue queue;
                vkGetDeviceQueue(m_LogicalDevice, m_QueueIndices[i], offsets[i], &queue);

                RHIQueueInfo info =
                {
                    (QueueType)i,
                    j
                };
                m_Queues[i][j] = std::make_shared<VulkanRHIQueue>(info, queue, m_QueueIndices[i]);
                RegisterResource(m_Queues[i][j]);

                offsets[i]++;
            }
        }
    }

	void VulkanDynamicRHI::CreateMemoryAllocator()
	{
        // volk集成vma: https://zhuanlan.zhihu.com/p/634912614 
        // vma官方文档: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/index.html
        // 对照该表，不同vulkan版本有不同的绑定要求: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/struct_vma_vulkan_functions.html

        VmaVulkanFunctions vulkanFunctions{};
        vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
        vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
        vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
        vulkanFunctions.vkFreeMemory = vkFreeMemory;
        vulkanFunctions.vkMapMemory = vkMapMemory;
        vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
        vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
        vulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
        vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
        vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
        vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
        vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
        vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
        vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
        vulkanFunctions.vkCreateImage = vkCreateImage;
        vulkanFunctions.vkDestroyImage = vkDestroyImage;
        vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
        // vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
        // vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;
        // vulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
        // vulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
        // vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
        vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
        vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
        vulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
        vulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2;
        vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
        // vulkanFunctions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
        // vulkanFunctions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion = VULKAN_VERSION;
        allocatorCreateInfo.physicalDevice = m_PhysicalDevice;
        allocatorCreateInfo.device = m_LogicalDevice;
        allocatorCreateInfo.instance = m_Instance;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // 加上这个才能获取地址

        VK_CHECK_RESULT(vmaCreateAllocator(&allocatorCreateInfo, &m_MemoryAllocator));
	}

	void VulkanDynamicRHI::CreateDescriptorPool()
	{
        std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 4096 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4096 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4096 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 4096 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 4096 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4096 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4096 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 4096 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 4096 },
        };

        //描述符池信息
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
        poolInfo.pPoolSizes = descriptorPoolSizes.data();
        poolInfo.maxSets = 8192;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;   // 使得描述符可以实时更新

        VK_CHECK_RESULT(vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool));
	}


    void VulkanDynamicRHI::CreateImmediateCommand()
    {
        m_ImmediateCommandContext = std::make_shared<VulkanRHICommandContextImmediate>();
        RegisterResource(m_ImmediateCommandContext);

        CommandListImmediateInfo info;
        info.context = m_ImmediateCommandContext;
        m_ImmediateCommandList = std::make_shared<RHICommandListImmediate>(info);
    }

    RHIQueueRef VulkanDynamicRHI::GetQueue(const RHIQueueInfo& info)
    {
        return m_Queues[info.type][info.index];

    }
    namespace Colors
    {
        // To experiment with editor theme live you can change these constexpr into static
        // members of a static "Theme" class and add a quick ImGui window to adjust the colour values
        namespace Theme
        {
            constexpr auto accent = IM_COL32(236, 158, 36, 255);
            constexpr auto highlight = IM_COL32(39, 185, 242, 255);
            constexpr auto niceBlue = IM_COL32(83, 232, 254, 255);
            constexpr auto compliment = IM_COL32(78, 151, 166, 255);
            constexpr auto background = IM_COL32(36, 36, 36, 255);
            constexpr auto backgroundDark = IM_COL32(26, 26, 26, 255);
            constexpr auto titlebar = IM_COL32(21, 21, 21, 255);
            constexpr auto titlebarOrange = IM_COL32(186, 66, 30, 255);
            constexpr auto titlebarGreen = IM_COL32(18, 88, 30, 255);
            constexpr auto titlebarRed = IM_COL32(185, 30, 30, 255);
            constexpr auto propertyField = IM_COL32(15, 15, 15, 255);
            constexpr auto text = IM_COL32(192, 192, 192, 255);
            constexpr auto textBrighter = IM_COL32(210, 210, 210, 255);
            constexpr auto textDarker = IM_COL32(128, 128, 128, 255);
            constexpr auto textError = IM_COL32(230, 51, 51, 255);
            constexpr auto muted = IM_COL32(77, 77, 77, 255);
            constexpr auto groupHeader = IM_COL32(47, 47, 47, 255);
            constexpr auto selection = IM_COL32(237, 192, 119, 255);
            constexpr auto selectionMuted = IM_COL32(237, 201, 142, 23);
            constexpr auto backgroundPopup = IM_COL32(50, 50, 50, 255);
            constexpr auto validPrefab = IM_COL32(82, 179, 222, 255);
            constexpr auto invalidPrefab = IM_COL32(222, 43, 43, 255);
            constexpr auto missingMesh = IM_COL32(230, 102, 76, 255);
            constexpr auto meshNotSet = IM_COL32(250, 101, 23, 255);
        }
    }
    void VulkanDynamicRHI::InitImGui(GLFWwindow* window)
    {

        VulkanUtil::VulkanRenderPassAttachments attachmentInfo = {};
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = VulkanUtil::RHIFormatToVkFormat(RHI_COLOR_FROMAT);
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentInfo.colorAttachments.push_back(colorAttachment);
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = VulkanUtil::RHIFormatToVkFormat(RHI_DEPTH_FROMAT);
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentInfo.depthStencilAttachment = depthAttachment;
        VkRenderPass tempPass = FindOrCreateVkRenderPass(attachmentInfo);

        std::shared_ptr<VulkanRHIQueue> queue = CAST<VulkanRHIQueue>(m_Queues[QUEUE_TYPE_GRAPHICS][0]);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows 
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        //ImGui::StyleColorsDark();
        auto& style = ImGui::GetStyle();
        auto& colors = ImGui::GetStyle().Colors;

        //========================================================
        /// Colours

        // Headers
        colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
        colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
        colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);

        // Buttons
        colors[ImGuiCol_Button] = ImColor(56, 56, 56, 200);
        colors[ImGuiCol_ButtonHovered] = ImColor(70, 70, 70, 255);
        colors[ImGuiCol_ButtonActive] = ImColor(56, 56, 56, 150);

        // Frame BG
        colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);
        colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);
        colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);

        // Tabs
        colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
        colors[ImGuiCol_TabHovered] = ImColor(255, 225, 135, 30);
        colors[ImGuiCol_TabActive] = ImColor(255, 225, 135, 60);
        colors[ImGuiCol_TabUnfocused] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
        colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabHovered];

        // Title
        colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
        colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Resize Grip
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);

        // Scrollbar
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);

        // Check Mark
        colors[ImGuiCol_CheckMark] = ImColor(200, 200, 200, 255);

        // Slider
        colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);

        // Text
        colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::text);

        // Checkbox
        colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::text);

        // Separator
        colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);
        colors[ImGuiCol_SeparatorActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::highlight);
        colors[ImGuiCol_SeparatorHovered] = ImColor(39, 185, 242, 150);

        // Window Background
        colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
        colors[ImGuiCol_ChildBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::background);
        colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundPopup);
        colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);

        // Tables
        colors[ImGuiCol_TableHeaderBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
        colors[ImGuiCol_TableBorderLight] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);

        // Menubar
        colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f };

        //========================================================
        /// Style
        style.FrameRounding = 2.5f;
        style.FrameBorderSize = 1.0f;
        style.IndentSpacing = 11.0f;
        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        //ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);
        auto funcLoader = [](const char* funcName, void* engine)
            {
                PFN_vkVoidFunction instanceAddr = vkGetInstanceProcAddr(VULKAN_INSTANCE, funcName);
                PFN_vkVoidFunction deviceAddr = vkGetDeviceProcAddr(VULKAN_DEVICE, funcName);
                return deviceAddr ? deviceAddr : instanceAddr;
            };
        const bool funcsLoaded = ImGui_ImplVulkan_LoadFunctions(funcLoader, this);

        ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = m_Instance;
        initInfo.PhysicalDevice = m_PhysicalDevice;
        initInfo.Device = m_LogicalDevice;
        initInfo.QueueFamily = queue->GetQueueFamilyIndex();
        initInfo.Queue = queue->GetHandle();
        initInfo.DescriptorPool = m_DescriptorPool;
        initInfo.PipelineCache = nullptr;
        initInfo.MinImageCount = FRAMES_IN_FLIGHT;
        initInfo.ImageCount = FRAMES_IN_FLIGHT;
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        ImGui_ImplVulkan_Init(&initInfo, tempPass);

        // Upload Fonts
        io.Fonts->AddFontFromFileTTF("Assets/Font/opensans/OpenSans-SemiBoldItalic.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
        RHI_DYNAMICRHI->GetImmediateCommandList(true);
        RHI_DYNAMICRHI->GetImmediateCommandList()->UploadImGuiFonts();
        RHI_DYNAMICRHI->GetImmediateCommandList()->Flush();
    }

	RHISurfaceRef VulkanDynamicRHI::CreateSurface(GLFWwindow* window)
	{
        RHISurfaceRef surface = std::make_shared<VulkanRHISurface>(window);
        RegisterResource(surface);

        return surface;
	}

	RHISwapchainRef VulkanDynamicRHI::CreateSwapChain(const RHISwapchainInfo& info)
	{
         RHISwapchainRef swapchain = std::make_shared<VulkanRHISwapchain>(info);
         RegisterResource(swapchain);

        return swapchain;
	}

	RHICommandPoolRef VulkanDynamicRHI::CreateCommandPool(const RHICommandPoolInfo& info)
	{
        RHICommandPoolRef commandPool = std::make_shared<VulkanRHICommandPool>(info);
        RegisterResource(commandPool);

        return commandPool;
	}

	RHICommandContextRef VulkanDynamicRHI::CreateCommandContext(RHICommandPoolRef pool)
	{
        RHICommandContextRef commandContext = std::make_shared<VulkanRHICommandContext>(pool);
        RegisterResource(commandContext);

        return commandContext;
	}

    RHIFenceRef VulkanDynamicRHI::CreateFence(bool signaled)
    {
        RHIFenceRef fence = std::make_shared<VulkanRHIFence>(signaled);
        RegisterResource(fence);

        return fence;
    }

    RHISemaphoreRef VulkanDynamicRHI::CreateSemaphore()
    {
        RHISemaphoreRef semaphore = std::make_shared<VulkanRHISemaphore>();
        RegisterResource(semaphore);

        return semaphore;
    }

	RHITextureRef VulkanDynamicRHI::CreateTexture(const RHITextureInfo& info)
	{
        RHITextureRef texture = std::make_shared<VulkanRHITexture>(info);
        RegisterResource(texture);

        return texture;
	}

	RHISamplerRef VulkanDynamicRHI::CreateSampler(const RHISamplerInfo& info)
	{
        RHISamplerRef sampler = std::make_shared<VulkanRHISampler>(info);
        RegisterResource(sampler);

        return sampler;
	}

	RHIShaderRef VulkanDynamicRHI::CreateShader(const RHIShaderInfo& info)
	{
        RHIShaderRef shader = std::make_shared<VulkanRHIShader>(info);
        RegisterResource(shader);

        return shader;
	}

	RHICommandListImmediateRef VulkanDynamicRHI::GetImmediateCommandList(bool start)
	{
        if (!m_ImmediateCommandList) {
            CreateImmediateCommand();
        }
        if (start) {
            CAST<VulkanRHICommandContextImmediate>(m_ImmediateCommandContext)->BeginSingleTimeCommand();
        }
        return m_ImmediateCommandList;

	}

	RHITextureViewRef VulkanDynamicRHI::CreateTextureView(const RHITextureViewInfo& info)
	{
        RHITextureViewRef textureView = std::make_shared<VulkanRHITextureView>(info);
        RegisterResource(textureView);

        return textureView;
	}

	RHIBufferRef VulkanDynamicRHI::CreateBuffer(const RHIBufferInfo& info)
	{
        RHIBufferRef buffer = std::make_shared<VulkanRHIBuffer>(info);
        RegisterResource(buffer);

        return buffer;
	}

	RHIRootSignatureRef VulkanDynamicRHI::CreateRootSignature(const RHIRootSignatureInfo& info)
	{
        RHIRootSignatureRef rootSignature = std::make_shared<VulkanRHIRootSignature>(info);
        RegisterResource(rootSignature);

        return rootSignature;
	}

	RHIGraphicsPipelineRef VulkanDynamicRHI::CreateGraphicsPipeline(const RHIGraphicsPipelineInfo& info)
	{
        RHIGraphicsPipelineRef graphicsPipeline = std::make_shared<VulkanRHIGraphicsPipeline>(info);
        RegisterResource(graphicsPipeline);

        return graphicsPipeline;
	}

	RHIRenderPassRef VulkanDynamicRHI::CreateRenderPass(const RHIRenderPassInfo& info)
	{
        RHIRenderPassRef renderPass = std::make_shared<VulkanRHIRenderPass>(info);
        RegisterResource(renderPass);

        return renderPass;
	}

    VkRenderPass VulkanDynamicRHI::CreateVkRenderPass(const VulkanUtil::VulkanRenderPassAttachments& info)
    {
        // 是否有 depth 附件
        bool hasDepth = (info.depthStencilAttachment.format != VK_FORMAT_UNDEFINED);

        std::vector<VkAttachmentDescription> attachments;
        std::vector<VkAttachmentReference> colorReferences;
        VkAttachmentReference depthReference = {};

        uint32_t attachmentIndex = 0;

        // --- 处理 color attachments（每个 color 一个 attachment + reference） ---
        for (const VkAttachmentDescription& src : info.colorAttachments)
        {
            VkAttachmentDescription desc = src; // 拷贝一份，避免修改原始结构
            // 为 color 附件选择合理的初始/最终 layout（可根据需要调整）
            if (desc.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
                desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 让驱动在开始时 transition（更通用）
            // finalLayout 默认设置为 COLOR_ATTACHMENT_OPTIMAL；如果这是 swapchain image，需要调用方替换为 PRESENT_SRC_KHR
            if (desc.finalLayout == VK_IMAGE_LAYOUT_UNDEFINED)
                desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // stencil 对 color 无效，但保留原来的 load/store 设置（如果应用需要，可以 override）
            // 把默认的 stencil ops 也设置为 color 的 load/store，以避免未初始化行为
            desc.stencilLoadOp = desc.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : desc.stencilLoadOp;
            desc.stencilStoreOp = desc.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE ? VK_ATTACHMENT_STORE_OP_DONT_CARE : desc.stencilStoreOp;

            attachments.push_back(desc);

            VkAttachmentReference cref{};
            cref.attachment = attachmentIndex;
            cref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorReferences.push_back(cref);

            ++attachmentIndex;
        }

        // --- 处理 depth stencil attachment（如果存在） ---
        if (hasDepth)
        {
            VkAttachmentDescription desc = info.depthStencilAttachment; // 拷贝
            // depth 初始/最终 layout：初始使用 UNDEFINED 更通用（表示可由 renderpass transition）
            if (desc.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
                desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            if (desc.finalLayout == VK_IMAGE_LAYOUT_UNDEFINED)
                desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            // stencil ops：通常 depth-stencil 需要设置 stencilLoad/Store 到合理值（这里保持与 loadOp/storeOp 一致）
            desc.stencilLoadOp = desc.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE ? desc.loadOp : desc.stencilLoadOp;
            desc.stencilStoreOp = desc.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE ? desc.storeOp : desc.stencilStoreOp;

            attachments.push_back(desc);

            depthReference.attachment = attachmentIndex;
            depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            ++attachmentIndex;
        }

        // --- 准备 subpass ---
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
        subpass.pColorAttachments = colorReferences.empty() ? nullptr : colorReferences.data();
        subpass.pDepthStencilAttachment = hasDepth ? &depthReference : nullptr;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;
        subpass.pResolveAttachments = nullptr; // 若支持 MSAA，请在此提供 resolve attachments

        // --- Subpass dependency：外部 -> subpass，保证 layout 转换/写入可见性 ---
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // --- RenderPass create info ---
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 0;
        renderPassInfo.pDependencies = nullptr;

        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkResult result = vkCreateRenderPass(m_LogicalDevice, &renderPassInfo, nullptr, &renderPass);
        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create render pass! VkResult = {0}", static_cast<int>(result));
            return VK_NULL_HANDLE;
        }

        return renderPass;
    }
    VkFramebuffer VulkanDynamicRHI::CreateVkFramebuffer(const VkFramebufferCreateInfo& info)
	{
        VkFramebuffer frameBuffer;
        if (vkCreateFramebuffer(m_LogicalDevice, &info, nullptr, &frameBuffer) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create framebuffer!");
        }

        return frameBuffer;
	}

	RHIDescriptorSetRef VulkanDynamicRHI::GetImGuiTextId(RHITextureViewRef textureView)
	{
        static RHISamplerRef sampler = nullptr; // 用同一个采样器即可
        if (!sampler) {
            RHISamplerInfo samplerInfo;
            samplerInfo.addressModeU = ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler = DYNAMICRHI->CreateSampler(samplerInfo);
        }

        VkDescriptorSet texId= ImGui_ImplVulkan_AddTexture((VkSampler)sampler->RawHandle(),(VkImageView)textureView->RawHandle(),VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        return std::make_shared<VulkanRHIDescriptorSet>(texId);
	}

    VulkanRHICommandContext::VulkanRHICommandContext(RHICommandPoolRef pool): RHICommandContext(pool)
    {
        this->pool = std::static_pointer_cast<VulkanRHICommandPool>(pool);
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = this->pool->GetHandle();
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(VULKAN_DEVICE, &allocInfo, &handle) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to allocate command buffer!");
        }

        // 耗时查询池
        VkQueryPoolCreateInfo queryPoolInfo{};
        queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolInfo.queryCount = 100; // 每个阶段需要 2 个查询（开始 + 结束）

        vkCreateQueryPool(VULKAN_DEVICE, &queryPoolInfo, nullptr, &m_TimestampQueryPool);
    }

	void VulkanRHICommandContext::BeginCommand()
	{
        vkResetCommandBuffer(handle, 0);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;

        vkBeginCommandBuffer(handle, &beginInfo);

        ResetQueryState();
       
	}

	void VulkanRHICommandContext::EndCommand()
	{
        if (vkEndCommandBuffer(handle) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to end command buffer!");
        }
	}
    void TextureBarrier(VkCommandBuffer commandBuffer, const RHITextureBarrier& barrier)
    {
        TextureSubresourceRange range = barrier.subresource;
        if (range.aspect == TEXTURE_ASPECT_NONE) range = barrier.texture->GetDefaultSubresourceRange();

        VkAccessFlags srcAccessMask = VulkanUtil::ResourceStateToAccessFlags(barrier.srcState);
        VkAccessFlags dstAccessMask = VulkanUtil::ResourceStateToAccessFlags(barrier.dstState);
        VkPipelineStageFlags srcStage = VulkanUtil::AccessFlagsToPipelineStageFlags(srcAccessMask);
        VkPipelineStageFlags dstStage = VulkanUtil::AccessFlagsToPipelineStageFlags(dstAccessMask);

        // srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;   // 可以保证绝对不会出错
        // dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;   // 目前验证层VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT还是会有一些报错，太难调了

        VkImageMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.oldLayout = VulkanUtil::ResourceStateToImageLayout(barrier.srcState);
        memoryBarrier.newLayout = VulkanUtil::ResourceStateToImageLayout(barrier.dstState);
        memoryBarrier.image = CAST<VulkanRHITexture>(barrier.texture)->GetHandle();
        memoryBarrier.subresourceRange = VulkanUtil::SubresourceToVk(range);
        memoryBarrier.srcAccessMask = srcAccessMask;
        memoryBarrier.dstAccessMask = dstAccessMask;

        vkCmdPipelineBarrier(
            commandBuffer,
            srcStage, dstStage, 0,
            0, nullptr,
            0, nullptr,
            1, &memoryBarrier);
    }
	void VulkanRHICommandContext::Execute(RHIFenceRef fence, RHISemaphoreRef waitSemaphore, RHISemaphoreRef signalSemaphore)
	{
        VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkFence signalFence = VK_NULL_HANDLE;

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &handle;

        if (fence != nullptr)
        {
            signalFence = CAST<VulkanRHIFence>(fence)->GetHandle();
        }
        if (waitSemaphore != nullptr)
        {
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &CAST<VulkanRHISemaphore>(waitSemaphore)->GetHandle();
            submitInfo.pWaitDstStageMask = &stage;
        }
        if (signalSemaphore != nullptr)
        {
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &CAST<VulkanRHISemaphore>(signalSemaphore)->GetHandle();
        }

        if (vkQueueSubmit(CAST<VulkanRHIQueue>(pool->GetQueue())->GetHandle(), 1, &submitInfo, signalFence) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to submit command buffer!");
        }
	}

	void VulkanRHICommandContext::BufferBarrier(const RHIBufferBarrier& barrier)
	{
        VkAccessFlags srcAccessMask = VulkanUtil::ResourceStateToAccessFlags(barrier.srcState);
        VkAccessFlags dstAccessMask = VulkanUtil::ResourceStateToAccessFlags(barrier.dstState);
        VkPipelineStageFlags srcStage = VulkanUtil::AccessFlagsToPipelineStageFlags(srcAccessMask);
        VkPipelineStageFlags dstStage = VulkanUtil::AccessFlagsToPipelineStageFlags(dstAccessMask);

        VkBufferMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.srcAccessMask = srcAccessMask;
        memoryBarrier.dstAccessMask = dstAccessMask;
        memoryBarrier.buffer = CAST<VulkanRHIBuffer>(barrier.buffer)->GetHandle();
        memoryBarrier.offset = barrier.offset;               // TODO
        memoryBarrier.size = barrier.size == 0 ? VK_WHOLE_SIZE : barrier.size;

        vkCmdPipelineBarrier(
            handle,
            srcStage, dstStage, 0,
            0, nullptr,
            1, &memoryBarrier,
            0, nullptr);
	}

    void VulkanRHICommandContext::BeginRenderPass(RHIRenderPassRef renderPass)
    {
        std::vector<VkClearValue> clearValues;
        for (uint32_t i = 0; i < MAX_RENDER_TARGETS; i++)
        {
            auto& attachment = CAST<VulkanRHIRenderPass>(renderPass)->GetInfo().colorAttachments[i];
            if (attachment.textureView == nullptr) break;

            VkClearValue clearValue = {};
            clearValue.color = { {attachment.clearColor.r, attachment.clearColor.g,
                                        attachment.clearColor.b, attachment.clearColor.a} };
            clearValues.push_back(clearValue);
        }
        const auto& depthAttachment = CAST<VulkanRHIRenderPass>(renderPass)->GetInfo().depthStencilAttachment;
        if (depthAttachment.textureView != nullptr)
        {
            VkClearValue clearValue = {};
            clearValue.depthStencil = { depthAttachment.clearDepth, depthAttachment.clearStencil };
            clearValues.push_back(clearValue);
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = CAST<VulkanRHIRenderPass>(renderPass)->GetHandle();
        renderPassInfo.framebuffer = CAST<VulkanRHIRenderPass>(renderPass)->GetFrameBuffer();
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = VulkanUtil::ExtentToVk(CAST<VulkanRHIRenderPass>(renderPass)->GetInfo().extent);
        renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(handle, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        this->renderPass = CAST<VulkanRHIRenderPass>(renderPass).get();
    }

    void VulkanRHICommandContext::EndRenderPass()
    {
        vkCmdEndRenderPass(handle);
        this->renderPass = nullptr;
    }
    void CopyTexture(VkCommandBuffer commandBuffer, RHITextureRef src, TextureSubresourceLayers srcSubresource, RHITextureRef dst, TextureSubresourceLayers dstSubresource)
    {
        VkImageCopy imageCopy = {};
        imageCopy.srcOffset = { 0, 0, 0 };    // TODO ?
        imageCopy.dstOffset = { 0, 0, 0 };
        imageCopy.srcSubresource = (srcSubresource.aspect == 0) ? VulkanUtil::SubresourceToVk(src->GetDefaultSubresourceLayers()) : VulkanUtil::SubresourceToVk(srcSubresource);
        imageCopy.dstSubresource = (dstSubresource.aspect == 0) ? VulkanUtil::SubresourceToVk(dst->GetDefaultSubresourceLayers()) : VulkanUtil::SubresourceToVk(dstSubresource);
        imageCopy.extent = VulkanUtil::ExtentToVk(src->MipExtent(srcSubresource.mipLevel));

        vkCmdCopyImage(commandBuffer,
            CAST<VulkanRHITexture>(src)->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            CAST<VulkanRHITexture>(dst)->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &imageCopy);


    }
	void VulkanRHICommandContext::CopyTexture(RHITextureRef src, TextureSubresourceLayers srcSubresource, RHITextureRef dst, TextureSubresourceLayers dstSubresource)
	{
        VkImageCopy imageCopy = {};
        imageCopy.srcOffset = { 0, 0, 0 };    // TODO ?
        imageCopy.dstOffset = { 0, 0, 0 };
        imageCopy.srcSubresource = (srcSubresource.aspect == 0) ? VulkanUtil::SubresourceToVk(src->GetDefaultSubresourceLayers()) : VulkanUtil::SubresourceToVk(srcSubresource);
        imageCopy.dstSubresource = (dstSubresource.aspect == 0) ? VulkanUtil::SubresourceToVk(dst->GetDefaultSubresourceLayers()) : VulkanUtil::SubresourceToVk(dstSubresource);
        imageCopy.extent = VulkanUtil::ExtentToVk(src->MipExtent(srcSubresource.mipLevel));

        vkCmdCopyImage(handle,
            CAST<VulkanRHITexture>(src)->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            CAST<VulkanRHITexture>(dst)->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &imageCopy);

	}
    // 假定处于正确的src在src状态，dst在dst状态
    void BlitTexture(VkCommandBuffer commandBuffer,
        RHITextureRef src, RHITextureRef dst,
        TextureSubresourceLayers srcSubresource, TextureSubresourceLayers dstSubresource,
        FilterType filter)
    {
        VkImageSubresourceLayers srcLayer = VulkanUtil::SubresourceToVk(srcSubresource);
        VkImageSubresourceLayers dstLayer = VulkanUtil::SubresourceToVk(dstSubresource);

        uint32_t srcMip = srcSubresource.mipLevel;
        uint32_t dstMip = dstSubresource.mipLevel;

        VkImageBlit blit = {};
        blit.srcOffsets[0] = { 0, 0, 0 }; //TODO offset
        blit.srcOffsets[1] = { (int32_t)(src->GetInfo().extent.width / pow(2, srcMip)),
                                (int32_t)(src->GetInfo().extent.height / pow(2, srcMip)), 1 };
        blit.srcSubresource = srcLayer;

        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { (int32_t)(dst->GetInfo().extent.width / pow(2, dstMip)),
                                (int32_t)(dst->GetInfo().extent.height / pow(2, dstMip)), 1 };
        blit.dstSubresource = dstLayer;

        vkCmdBlitImage(commandBuffer,
            CAST<VulkanRHITexture>(src)->GetHandle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            CAST<VulkanRHITexture>(dst)->GetHandle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blit,
            VulkanUtil::FilterTypeToVk(filter));
    }

	void VulkanRHICommandContext::GenerateMips(RHITextureRef src)
	{
        //总计生成的mip层数
        uint32_t mipLevels = src->GetInfo().mipLevels;

        VkImageSubresourceRange transition = {};
        transition.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            transition.baseMipLevel = 0;
        transition.levelCount = 1;
        transition.baseArrayLayer = 0;
        transition.layerCount = 1;

        for (uint32_t i = 0; i < src->GetInfo().arrayLayers; i++)
        {
            transition.baseMipLevel = 0;
            transition.baseArrayLayer = i;

            // 先将后面的层全设置到dst
            TextureBarrier(
                { src,
                RESOURCE_STATE_TRANSFER_SRC, RESOURCE_STATE_TRANSFER_DST,
                        {TEXTURE_ASPECT_COLOR, 1, mipLevels - 1, transition.baseArrayLayer, transition.layerCount} });

            //循环生成各级mip，并将对应层级转到srcLayout
            for (uint32_t i = 1; i < mipLevels; i++)    //总共mipLevels级，只需要mipLevels-1次blit
            {
                BlitTexture(
                    handle,
                    src,
                    src,
                    { transition.aspectMask, transition.baseMipLevel, transition.baseArrayLayer, transition.layerCount },
                    { transition.aspectMask, transition.baseMipLevel + 1, transition.baseArrayLayer, transition.layerCount },
                    FILTER_TYPE_LINEAR);

                // 将生成后的层级设置到src
                TextureBarrier(
                    { src,
                    RESOURCE_STATE_TRANSFER_DST, RESOURCE_STATE_TRANSFER_SRC,
                            {TEXTURE_ASPECT_COLOR, transition.baseMipLevel + 1, 1, transition.baseArrayLayer, transition.layerCount} });

                transition.baseMipLevel++;
            }
        }
	}



	VulkanRHICommandContextImmediate::VulkanRHICommandContextImmediate()
	{
        fence = VULKAN_RHI->CreateFence(true);
        queue = VULKAN_RHI->GetQueue({ QUEUE_TYPE_GRAPHICS, 0 });
        commandPool = VULKAN_RHI->CreateCommandPool({ queue });
	}

	void VulkanRHICommandContextImmediate::TextureBarrier(const RHITextureBarrier& barrier)
	{
        TextureSubresourceRange range = barrier.subresource;
        if (range.aspect == TEXTURE_ASPECT_NONE) range = barrier.texture->GetDefaultSubresourceRange();

        VkAccessFlags srcAccessMask = VulkanUtil::ResourceStateToAccessFlags(barrier.srcState);
        VkAccessFlags dstAccessMask = VulkanUtil::ResourceStateToAccessFlags(barrier.dstState);
        VkPipelineStageFlags srcStage = VulkanUtil::AccessFlagsToPipelineStageFlags(srcAccessMask);
        VkPipelineStageFlags dstStage = VulkanUtil::AccessFlagsToPipelineStageFlags(dstAccessMask);

        VkImageMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.oldLayout = VulkanUtil::ResourceStateToImageLayout(barrier.srcState);
        memoryBarrier.newLayout = VulkanUtil::ResourceStateToImageLayout(barrier.dstState);
        memoryBarrier.image = CAST<VulkanRHITexture>(barrier.texture)->GetHandle();
        memoryBarrier.subresourceRange = VulkanUtil::SubresourceToVk(range);
        memoryBarrier.srcAccessMask = srcAccessMask;
        memoryBarrier.dstAccessMask = dstAccessMask;

        vkCmdPipelineBarrier(
            handle,
            srcStage, dstStage, 0,
            0, nullptr,
            0, nullptr,
            1, &memoryBarrier);
	}
    void VulkanRHICommandContext::TextureBarrier(const RHITextureBarrier& barrier)
    {
        /*
            srcStageMask/dstStageMask：PipelineStage（表示Pipeline的各个阶段），这两个设置表示src阶段命令完成之前，dst阶段不能开始执行（任务A需要执行完srcStage，任务B才能开始执行dstStage）
            例如：A->Barrier->B,这样提交命令后，B会停在dstStage，A的srcStage执行完成后，B才能继续
        */
        TextureSubresourceRange range = barrier.subresource;
        if (range.aspect == TEXTURE_ASPECT_NONE) range = barrier.texture->GetDefaultSubresourceRange();

        VkAccessFlags srcAccessMask = VulkanUtil::ResourceStateToAccessFlags(barrier.srcState);
        VkAccessFlags dstAccessMask = VulkanUtil::ResourceStateToAccessFlags(barrier.dstState);
        VkPipelineStageFlags srcStage = VulkanUtil::AccessFlagsToPipelineStageFlags(srcAccessMask);
        VkPipelineStageFlags dstStage = VulkanUtil::AccessFlagsToPipelineStageFlags(dstAccessMask);

        // srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;   // 可以保证绝对不会出错
        // dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;   // 目前验证层VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT还是会有一些报错，太难调了

        VkImageMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.oldLayout = VulkanUtil::ResourceStateToImageLayout(barrier.srcState);
        memoryBarrier.newLayout = VulkanUtil::ResourceStateToImageLayout(barrier.dstState);
        memoryBarrier.image = CAST<VulkanRHITexture>(barrier.texture)->GetHandle();
        memoryBarrier.subresourceRange = VulkanUtil::SubresourceToVk(range);   // 可以只转换部分层
        memoryBarrier.srcAccessMask = srcAccessMask;
        memoryBarrier.dstAccessMask = dstAccessMask;

        vkCmdPipelineBarrier(
            handle,
            srcStage, dstStage, 0,
            0, nullptr,
            0, nullptr,
            1, &memoryBarrier);
    
    }

	void VulkanRHICommandContext::SetViewport(Offset2D min, Offset2D max)
	{
        VkViewport viewport{};
        viewport.x = (float)min.x;
        viewport.y = (float)min.y;
        viewport.width = (float)(max.x - min.x);
        viewport.height = (float)(max.y - min.y);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(handle, 0, 1, &viewport);
	}

	void VulkanRHICommandContext::SetScissor(Offset2D min, Offset2D max)
	{
        VkRect2D scissor{};
        scissor.offset = { (int32_t)min.x, (int32_t)min.y };
        scissor.extent = { (uint32_t)(max.x - min.x), (uint32_t)(max.y - min.y) };
        vkCmdSetScissor(handle, 0, 1, &scissor);
	}
    void VulkanRHICommandContext::SetDepthBias(float constantBias, float slopeBias, float clampBias)
    {
        vkCmdSetDepthBias(handle, constantBias, clampBias, slopeBias);
    }

    void VulkanRHICommandContext::SetLineWidth(float width)
    {
        vkCmdSetLineWidth(handle, width);
    }

    void VulkanRHICommandContext::SetGraphicsPipeline(RHIGraphicsPipelineRef graphicsPipeline)
    {
        this->graphicsPipeline = CAST<VulkanRHIGraphicsPipeline>(graphicsPipeline).get();
        this->computePipeline = nullptr;
        // this->rayTraycingPipeline = nullptr;

        this->graphicsPipeline->Bind(handle);
    }

    void VulkanRHICommandContext::SetComputePipeline(RHIComputePipelineRef computePipeline)
    {
        this->graphicsPipeline = nullptr;
        this->computePipeline = CAST<VulkanRHIComputePipeline>(computePipeline).get();
        // this->rayTraycingPipeline = nullptr;

        this->computePipeline->Bind(handle);
    }

   /* void VulkanRHICommandContext::SetRayTracingPipeline(RHIRayTracingPipelineRef rayTracingPipeline)
    {
        this->graphicsPipeline = nullptr;
        this->computePipeline = nullptr;
        this->rayTraycingPipeline = ResourceCast(rayTracingPipeline).get();

        this->rayTraycingPipeline->Bind(handle);
    }*/

    void VulkanRHICommandContext::PushConstants(void* data, uint16_t size, ShaderFrequency frequency)
    {
        vkCmdPushConstants(handle,
            GetCuttentPipelineLayout(),
            VulkanUtil::ShaderFrequencyToVkStageFlags(frequency),
            0, size, data);
    }
    VkPipelineLayout VulkanRHICommandContext::GetCuttentPipelineLayout()
    {
        if (graphicsPipeline != nullptr)    return graphicsPipeline->GetPipelineLayout();
        if (computePipeline != nullptr)     return computePipeline->GetPipelineLayout();
        // if (rayTraycingPipeline != nullptr) return rayTraycingPipeline->GetPipelineLayout();

        LOG_ERROR("Havent bind any pipeline!"); return nullptr;
    }

    VkPipelineBindPoint VulkanRHICommandContext::GetCuttentBindingPoint()
    {
        if (graphicsPipeline != nullptr)        return VK_PIPELINE_BIND_POINT_GRAPHICS;
        if (computePipeline != nullptr)         return VK_PIPELINE_BIND_POINT_COMPUTE;
        // if (rayTraycingPipeline != nullptr)     return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;

        LOG_ERROR("Havent bind any pipeline!"); return VK_PIPELINE_BIND_POINT_MAX_ENUM;
    }

    void VulkanRHICommandContext::BindDescriptorSet(RHIDescriptorSetRef descriptor, uint32_t set)
    {
        vkCmdBindDescriptorSets(handle,
            GetCuttentBindingPoint(),
            GetCuttentPipelineLayout(),
            set, 1, &CAST<VulkanRHIDescriptorSet>(descriptor)->GetHandle(),
            0,              //TODO dynamic offset
            nullptr);
    }

    void VulkanRHICommandContext::BindVertexBuffer(RHIBufferRef vertexBuffer, uint32_t streamIndex, uint32_t offset)
    {
        VkDeviceSize offsets = offset;
        vkCmdBindVertexBuffers(handle, streamIndex, 1, &CAST<VulkanRHIBuffer>(vertexBuffer)->GetHandle(), &offsets);
    }

    void VulkanRHICommandContext::BindIndexBuffer(RHIBufferRef indexBuffer, uint32_t offset)
    {
        vkCmdBindIndexBuffer(handle, CAST<VulkanRHIBuffer>(indexBuffer)->GetHandle(), offset, VK_INDEX_TYPE_UINT32);    // 固定了索引用32位的
    }

    void VulkanRHICommandContext::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        vkCmdDispatch(handle, groupCountX, groupCountY, groupCountZ);
    }

    void VulkanRHICommandContext::DispatchIndirect(RHIBufferRef argumentBuffer, uint32_t argumentOffset)
    {
        LOG_ERROR("VulkanRHICommandContext::DispatchIndirect is not implemented yet!");
    }

    void VulkanRHICommandContext::TraceRays(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        //assert(this->rayTraycingPipeline != nullptr);

        ////gl_LaunchSizeEXT 对应此处给出的3维尺寸
        ////gl_LaunchIDEXT 类似于compute shader的gl_GlobalInvocationID，对应调用shader的坐标(ID)
        //vkCmdTraceRaysKHR(
        //    handle,
        //    &this->rayTraycingPipeline->GetRaygenRegion(),
        //    &this->rayTraycingPipeline->GetRayMissRegion(),
        //    &this->rayTraycingPipeline->GetHitRegion(),
        //    &this->rayTraycingPipeline->GetCallableRegion(),
        //    groupCountX,
        //    groupCountY,
        //    groupCountZ);
    }

    void VulkanRHICommandContext::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        vkCmdDraw(handle, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanRHICommandContext::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
    {
        vkCmdDrawIndexed(handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanRHICommandContext::DrawIndirect(RHIBufferRef argumentBuffer, uint32_t offset, uint32_t drawCount)
    {
        vkCmdDrawIndirect(handle, CAST<VulkanRHIBuffer> (argumentBuffer)->GetHandle(), offset, drawCount, sizeof(RHIIndirectCommand));
    }

    void VulkanRHICommandContext::DrawIndexedIndirect(RHIBufferRef argumentBuffer, uint32_t offset, uint32_t drawCount)
    {
        vkCmdDrawIndexedIndirect(handle, CAST<VulkanRHIBuffer>(argumentBuffer)->GetHandle(), offset, drawCount, sizeof(RHIIndexedIndirectCommand));
    }

    void VulkanRHICommandContext::ImGuiRenderDrawData()
    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), handle);
    }

	void VulkanRHICommandContext::PushLabel(const std::string& name, Color3 color)
	{
        uint32_t startIndex = m_TimestampQueryIndex;
        vkCmdWriteTimestamp(handle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_TimestampQueryPool, startIndex);
        m_TimestampQueryIndex++;

        VkDebugUtilsLabelEXT label_info;

        label_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label_info.pNext = nullptr;
        label_info.pLabelName = name.c_str();
        label_info.color[0] = color.r;
        label_info.color[1] = color.g;
        label_info.color[2] = color.b;
        label_info.color[3] = 1.0f;

        vkCmdBeginDebugUtilsLabelEXT(handle, &label_info);

        m_ActiveLabels.push_back({ name, startIndex, 0, 0.0f });
	}

	void VulkanRHICommandContext::PopLabel()
	{
        // 记录结束时间戳
        uint32_t endIndex = m_TimestampQueryIndex;
        vkCmdWriteTimestamp(handle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_TimestampQueryPool, endIndex);
        m_TimestampQueryIndex++;
        vkCmdEndDebugUtilsLabelEXT(handle);
        RHIGPUTimeInfo finishedLabel = m_ActiveLabels.back();
        m_ActiveLabels.pop_back();
        finishedLabel.EndQueryIndex = endIndex;
        m_FinishedLabels.push_back(finishedLabel);
	}
    void VulkanRHICommandContext::ResetQueryState()
    {
        m_TimestampQueryIndex = 0;
        m_FinishedLabels.clear();
        vkCmdResetQueryPool(handle, m_TimestampQueryPool, 0, 100);
    }
	std::vector<RHIGPUTimeInfo> VulkanRHICommandContext::GetGPUTime()
	{
        std::vector<RHIGPUTimeInfo> result;

        if (m_FinishedLabels.empty() || m_TimestampQueryIndex == 0)
        {
            return result;
        }

        std::vector<uint64_t> timestamps(m_TimestampQueryIndex);
        VkResult vkResult = vkGetQueryPoolResults(VULKAN_DEVICE,
            m_TimestampQueryPool,
            0,
            m_TimestampQueryIndex,
            sizeof(uint64_t) * m_TimestampQueryIndex,
            timestamps.data(),
            sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
        );

        if (vkResult != VK_SUCCESS)
        {
            LOG_ERROR("Failed to get query pool results! Vulkan Result: {}", vkResult);
            return result;
        }

        const double NANOSECONDS_TO_MILLISECONDS = 1'000'000.0;
        for (auto& labelInfo : m_FinishedLabels)
        {
            uint64_t startTimeNs = timestamps[labelInfo.StartQueryIndex];
            uint64_t endTimeNs = timestamps[labelInfo.EndQueryIndex];

            if (endTimeNs > startTimeNs)
            {
                labelInfo.DurationMs = static_cast<float>((endTimeNs - startTimeNs) / NANOSECONDS_TO_MILLISECONDS);
            }

            result.push_back(labelInfo);
        }

        return result;
	}

	void VulkanRHICommandContextImmediate::Flush()
	{
        EndSingleTimeCommand();
	}

	void VulkanRHICommandContextImmediate::BeginSingleTimeCommand()
	{
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = CAST<VulkanRHICommandPool>(commandPool)->GetHandle();
        allocInfo.commandBufferCount = 1;

        vkAllocateCommandBuffers(VULKAN_DEVICE, &allocInfo, &handle);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(handle, &beginInfo);
	}

	void VulkanRHICommandContextImmediate::EndSingleTimeCommand()
	{
        // 等待前一次flush执行完成
        fence->Wait();
        if (oldHandle != VK_NULL_HANDLE) vkFreeCommandBuffers(VULKAN_DEVICE, CAST<VulkanRHICommandPool>(commandPool)->GetHandle(), 1, &oldHandle);

        // 提交最新一次的flush
        if (vkEndCommandBuffer(handle) != VK_SUCCESS) {
            LOG_ERROR("Failed to record command buffer!");
        }

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &handle;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = nullptr;
        submitInfo.pWaitDstStageMask = nullptr;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = nullptr;

        VkResult result = vkQueueSubmit(CAST<VulkanRHIQueue>(queue)->GetHandle(), 1, &submitInfo, CAST<VulkanRHIFence>(fence)->GetHandle());
        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Failed to submit draw command buffer! [%d]", result);
        }

        oldHandle = handle;
	}
    void GenerateMipsFun(VkCommandBuffer commandBuffer, RHITextureRef src)
    {
        //总计生成的mip层数
        uint32_t mipLevels = src->GetInfo().mipLevels;

        VkImageSubresourceRange transition = {};
        transition.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            transition.baseMipLevel = 0;
        transition.levelCount = 1;
        transition.baseArrayLayer = 0;
        transition.layerCount = 1;

        for (uint32_t i = 0; i < src->GetInfo().arrayLayers; i++)
        {
            transition.baseMipLevel = 0;
            transition.baseArrayLayer = i;

            // 先将后面的层全设置到dst
            TextureBarrier(commandBuffer,
                { src,
                RESOURCE_STATE_TRANSFER_SRC, RESOURCE_STATE_TRANSFER_DST,
                        {TEXTURE_ASPECT_COLOR, 1, mipLevels - 1, transition.baseArrayLayer, transition.layerCount} });

            //循环生成各级mip，并将对应层级转到srcLayout
            for (uint32_t i = 1; i < mipLevels; i++)    //总共mipLevels级，只需要mipLevels-1次blit
            {
                BlitTexture(
                    commandBuffer,
                    src,
                    src,
                    { transition.aspectMask, transition.baseMipLevel, transition.baseArrayLayer, transition.layerCount },
                    { transition.aspectMask, transition.baseMipLevel + 1, transition.baseArrayLayer, transition.layerCount },
                    FILTER_TYPE_LINEAR);

                // 将生成后的层级设置到src
                TextureBarrier(commandBuffer,
                    { src,
                    RESOURCE_STATE_TRANSFER_DST, RESOURCE_STATE_TRANSFER_SRC,
                            {TEXTURE_ASPECT_COLOR, transition.baseMipLevel + 1, 1, transition.baseArrayLayer, transition.layerCount} });

                transition.baseMipLevel++;
            }
        }
    }
	void VulkanRHICommandContextImmediate::GenerateMips(RHITextureRef src)
	{
        GenerateMipsFun(handle, src);
	}

    void VulkanRHICommandContextImmediate::ImGuiUploadFonts()
    {
        ImGui_ImplVulkan_CreateFontsTexture(handle);

    }

	void VulkanRHICommandContextImmediate::CopyBufferToTexture(RHIBufferRef src, uint64_t srcOffset, RHITextureRef dst, TextureSubresourceLayers dstSubresource)
	{
        VkBufferImageCopy copy = {};
        copy.bufferOffset = srcOffset;
        copy.bufferRowLength = 0;
        copy.bufferImageHeight = 0;
        copy.imageSubresource = VulkanUtil::SubresourceToVk(dstSubresource);
        copy.imageOffset = { 0, 0, 0 };
        copy.imageExtent = VulkanUtil::ExtentToVk(dst->MipExtent(dstSubresource.mipLevel));

        vkCmdCopyBufferToImage(handle,
            CAST<VulkanRHIBuffer>(src)->GetHandle(),
            CAST<VulkanRHITexture>(dst)->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copy);
	}

}













