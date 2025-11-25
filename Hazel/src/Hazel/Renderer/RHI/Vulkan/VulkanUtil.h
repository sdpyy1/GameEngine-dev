#pragma once
#include "Volk/volk.h"
#include <Hazel/Core/macro.h>
#include <GLFW/glfw3.h>
#include "spirv_reflect.h"
#include "Hazel/Renderer/RHI/RHIBase.h"
namespace GameEngine {
#define DYNAMICRHI DynamicRHI::Get()
#define VULKAN_RHI std::static_pointer_cast<VulkanDynamicRHI>(DynamicRHI::Get()).get()
#define VULKAN_DESCPOOL std::static_pointer_cast<VulkanDynamicRHI>(DynamicRHI::Get()).get()->GetDescriptorPool()
#define VULKAN_INSTANCE (std::static_pointer_cast<VulkanDynamicRHI>(DynamicRHI::Get()).get())->GetInstance()
#define VULKAN_PHYSICALDEVICE (std::static_pointer_cast<VulkanDynamicRHI>(DynamicRHI::Get()).get())->GetPhysicalDevice()
#define VULKAN_DEVICE (std::static_pointer_cast<VulkanDynamicRHI>(DynamicRHI::Get()).get())->GetDevice()
#define VULKAN_VMA (std::static_pointer_cast<VulkanDynamicRHI>(DynamicRHI::Get()).get())->GetVMA()
#define VULKAN_QUERYPOOL std::static_pointer_cast<VulkanDynamicRHI>(DynamicRHI::Get()).get()->GetQueryPool()
#define CAST std::static_pointer_cast
#define VK_CHECK_RESULT(f)\
{\
	VkResult res = (f);\
	VulkanUtil::VulkanCheckResult(res, __FILE__, __LINE__);ASSERT(res == VK_SUCCESS);\
}
    static const char* INSTANCE_LAYERS[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    static const char* DEVICE_LAYERS[] = {
        "废弃",
    };

    // TODO: 优先级
    static const float QUEUE_PRIORITIES[] = {
        1.f, 1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f, 1.f,
    };
#ifdef HZ_PLATFORM_WINDOWS
#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"
#endif

    static const char* INSTANCE_EXTENTIONS[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
    };
    static const char* DEVICE_EXTENTIONS[] = {
         VK_KHR_SWAPCHAIN_EXTENSION_NAME,
         VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
        "VK_EXT_vertex_input_dynamic_state"
    };
    static const char* RAY_TRACING_DEVICE_EXTENTIONS[] = {
        // Ray tracing related extensions required
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,

        // Required by VK_KHR_acceleration_structure   
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,

        // Required for VK_KHR_ray_tracing_pipeline
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,

        // Required by VK_KHR_spirv_1_4
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    };
    // 验证层的功能
    static const std::vector<VkValidationFeatureEnableEXT> ENABLED_VALIDATION_FEATURES = {  // 需要时再启用，对帧数影响大
#if ENABLE_DEBUG_MODE
        //VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        //VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
#endif
    };


    static const char* INSTANCE_VALIDATION_EXTENTIONS[] = {
        VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME
    };
    static const char* TARGET_DEVICES[] = {
        "NVIDIA",
        "AMD"
    };
	namespace VulkanUtil {
        static bool IsExtensionSupported(const std::vector<std::string>& allExt, const std::string& extensionName)
        {
            for (const auto& ext : allExt) {
                if (ext == extensionName) {
                    return true;
                }
            }
            return false;
        }

        // 验证层Debug回调
        static VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo()
        {
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
            debugCreateInfo.pfnUserCallback =
                [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                    void* pUserData) -> VkBool32
                {
                    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
                        LOG_ERROR_TAG("Vulkan Validation Error", pCallbackData->pMessage);
                    }
                    return VK_FALSE;
                };
            return debugCreateInfo;
        }

        // 验证层功能设置
        static VkValidationFeaturesEXT GetValidationFeatureCreateInfo()
        {
            VkValidationFeaturesEXT validationFeaturesExt = {};
            validationFeaturesExt.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            validationFeaturesExt.pEnabledValidationFeatures = ENABLED_VALIDATION_FEATURES.data();
            validationFeaturesExt.enabledValidationFeatureCount = ENABLED_VALIDATION_FEATURES.size();

            return validationFeaturesExt;
        }

        // 设置实例扩展
        static std::vector<const char*> GetRequiredInstanceExtensions(bool debug)
        {
            std::vector<const char*> extensions;

            unsigned int glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            for (unsigned int i = 0; i < glfwExtensionCount; i++) {
                extensions.push_back(glfwExtensions[i]);
            }

            for (auto extention : INSTANCE_EXTENTIONS)  extensions.push_back(extention);
            if (debug) for (auto extention : INSTANCE_VALIDATION_EXTENTIONS)  extensions.push_back(extention);

            return extensions;
        }
        static VkFilter FilterTypeToVk(FilterType filterType)
        {
            VkFilter filter;
            switch (filterType) {
            case FILTER_TYPE_NEAREST:   filter = VK_FILTER_NEAREST;     break;
            case FILTER_TYPE_LINEAR:    filter = VK_FILTER_LINEAR;      break;
            default:                    filter = VK_FILTER_LINEAR;      break;
            }

            return  filter;
        }
        static VkSamplerAddressMode AddressModeToVk(AddressMode addressMode)
        {
            VkSamplerAddressMode mode;
            switch (addressMode) {
            case ADDRESS_MODE_MIRROR:               mode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;     break;
            case ADDRESS_MODE_REPEAT:               mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;              break;
            case ADDRESS_MODE_CLAMP_TO_EDGE:        mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;       break;
            case ADDRESS_MODE_CLAMP_TO_BORDER:      mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;     break;
            default:                                mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;              break;
            }

            return mode;
        }
        static VkAccessFlags ResourceStateToAccessFlags(RHIResourceState state)
        {
            // 各个资源状态决定了访问用途，作为src和dst是一致的
            VkAccessFlags accessFlags = VK_ACCESS_NONE;
            switch (state) {
                case RESOURCE_STATE_UNDEFINED:                      accessFlags = VK_ACCESS_NONE;                                           break;     // 无效？
                case RESOURCE_STATE_COMMON:                         accessFlags = VK_ACCESS_NONE;                                           break;     // 无效？   
                case RESOURCE_STATE_TRANSFER_SRC:                   accessFlags = VK_ACCESS_TRANSFER_READ_BIT;                              break;
                case RESOURCE_STATE_TRANSFER_DST:                   accessFlags = VK_ACCESS_TRANSFER_WRITE_BIT;                             break;
                case RESOURCE_STATE_VERTEX_BUFFER:                  accessFlags = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;                      break;
                case RESOURCE_STATE_INDEX_BUFFER:                   accessFlags = VK_ACCESS_INDEX_READ_BIT;                                 break;
                case RESOURCE_STATE_COLOR_ATTACHMENT:               accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;                     break;
                case RESOURCE_STATE_DEPTH_STENCIL_ATTACHMENT:       accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;             break;
                case RESOURCE_STATE_UNORDERED_ACCESS:               accessFlags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;   break;
                case RESOURCE_STATE_SHADER_RESOURCE:                accessFlags = VK_ACCESS_SHADER_READ_BIT;                                break;
                case RESOURCE_STATE_INDIRECT_ARGUMENT:              accessFlags = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;                      break;
                case RESOURCE_STATE_PRESENT:                        accessFlags = VK_ACCESS_NONE;                                           break;      // 无效？ 
            // case RESOURCE_STATE_ACCELERATION_STRUCTURE:         accessFlags = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;   break;
            default:                                            LOG_ERROR("Unsupported resource state!");
            }
            return accessFlags;
        }

        static VkImageLayout ResourceStateToImageLayout(RHIResourceState state)
        {
            // 各个资源状态决定了布局，作为src和dst是一致的
            VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
            switch (state) {
            case RESOURCE_STATE_UNDEFINED:                      layout = VK_IMAGE_LAYOUT_UNDEFINED;                         break;
            case RESOURCE_STATE_COMMON:                         layout = VK_IMAGE_LAYOUT_GENERAL;                           break;
            case RESOURCE_STATE_TRANSFER_SRC:                   layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;              break;
            case RESOURCE_STATE_TRANSFER_DST:                   layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;              break;
            case RESOURCE_STATE_COLOR_ATTACHMENT:               layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;          break;
            case RESOURCE_STATE_DEPTH_STENCIL_ATTACHMENT:       layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  break;
            case RESOURCE_STATE_UNORDERED_ACCESS:               layout = VK_IMAGE_LAYOUT_GENERAL;                           break;
            case RESOURCE_STATE_SHADER_RESOURCE:                layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;          break;
            case RESOURCE_STATE_PRESENT:                        layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;                   break;
            default:                                            LOG_ERROR("Unsupported resource state!");
            }
            return layout;
        }
        static VkPipelineStageFlags AccessFlagsToPipelineStageFlags(VkAccessFlags accessFlags)
        {
            // 根据所有的accessFlags来分析涉及到的stage阶段 参考Sakura

            VkPipelineStageFlags flags = 0;

            if (accessFlags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT)               flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;                    // 0x00000002

            if (accessFlags & (VK_ACCESS_INDEX_READ_BIT |
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT))           flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;                     // 0x00000004

            if (accessFlags & (VK_ACCESS_UNIFORM_READ_BIT |VK_ACCESS_SHADER_READ_BIT |VK_ACCESS_SHADER_WRITE_BIT)) {
                flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |                   // 0x00000008
                    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |     // 0x00000010
                    VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |  // 0x00000020
                    VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |                 // 0x00000040
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |                 // 0x00000080
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                if (DYNAMICRHI->isEnableRayTracing()) {
                    flags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
                }
            }// 0x00200000

            if (accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT)            flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;                  // 0x00000080

            if (accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT))  flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |            // 0x00000100
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;              // 0x00000200

            if (accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT))          flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;          // 0x00000400 

            if (accessFlags & (VK_ACCESS_TRANSFER_READ_BIT |
                VK_ACCESS_TRANSFER_WRITE_BIT))                  flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;                         // 0x00001000

            if (accessFlags & (VK_ACCESS_HOST_READ_BIT |
                VK_ACCESS_HOST_WRITE_BIT))                      flags |= VK_PIPELINE_STAGE_HOST_BIT;                             // 0x00004000


            if (flags == 0) flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            return flags;
        }

        static VkSamplerMipmapMode MipMapModeToVk(MipMapMode mipMapMode)
        {
            VkSamplerMipmapMode mode;
            switch (mipMapMode) {
            case MIPMAP_MODE_NEAREST:   mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;  break;
            case MIPMAP_MODE_LINEAR:    mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;   break;
            default:                    mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;  break;
            }

            return mode;
        }
        static VkSamplerReductionMode SamplerReductionModeToVk(SamplerReductionMode reductionMode)
        {
            VkSamplerReductionMode mode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
            switch (reductionMode) {
            case SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE:   mode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE; break;
            case SAMPLER_REDUCTION_MODE_MIN:                mode = VK_SAMPLER_REDUCTION_MODE_MIN; break;
            case SAMPLER_REDUCTION_MODE_MAX:                mode = VK_SAMPLER_REDUCTION_MODE_MAX; break;
            default:                                        mode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE; break;
            }

            return mode;
        }
        static VkShaderStageFlagBits ShaderFrequencyToVkStageFlagBits(ShaderFrequency frequency)
        {
            if (frequency & SHADER_FREQUENCY_COMPUTE)        return VK_SHADER_STAGE_COMPUTE_BIT;
            if (frequency & SHADER_FREQUENCY_VERTEX)         return VK_SHADER_STAGE_VERTEX_BIT;
            if (frequency & SHADER_FREQUENCY_FRAGMENT)       return VK_SHADER_STAGE_FRAGMENT_BIT;
            if (frequency & SHADER_FREQUENCY_GEOMETRY)       return VK_SHADER_STAGE_GEOMETRY_BIT;
            if (frequency & SHADER_FREQUENCY_RAY_GEN)        return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            if (frequency & SHADER_FREQUENCY_CLOSEST_HIT)    return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            if (frequency & SHADER_FREQUENCY_RAY_MISS)       return VK_SHADER_STAGE_MISS_BIT_KHR;
            if (frequency & SHADER_FREQUENCY_INTERSECTION)   return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            if (frequency & SHADER_FREQUENCY_ANY_HIT)        return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            if (frequency & SHADER_FREQUENCY_MESH)           return VK_SHADER_STAGE_MESH_BIT_EXT;
            LOG_ERROR("Unsupported frequency!");      return VK_SHADER_STAGE_ALL;
        }

        static VkCompareOp CompareFunctionToVk(CompareFunction compareFunction)
        {
            VkCompareOp compare;
            switch (compareFunction) {
            case COMPARE_FUNCTION_LESS:             compare = VK_COMPARE_OP_LESS;               break;
            case COMPARE_FUNCTION_LESS_EQUAL:       compare = VK_COMPARE_OP_LESS_OR_EQUAL;      break;
            case COMPARE_FUNCTION_GREATER:          compare = VK_COMPARE_OP_GREATER;            break;
            case COMPARE_FUNCTION_GREATER_EQUAL:    compare = VK_COMPARE_OP_GREATER_OR_EQUAL;   break;
            case COMPARE_FUNCTION_EQUAL:            compare = VK_COMPARE_OP_EQUAL;              break;
            case COMPARE_FUNCTION_NOT_EQUAL:        compare = VK_COMPARE_OP_NOT_EQUAL;          break;
            case COMPARE_FUNCTION_NEVER:            compare = VK_COMPARE_OP_NEVER;              break;
            case COMPARE_FUNCTION_ALWAYS:           compare = VK_COMPARE_OP_ALWAYS;             break;
            default:                                compare = VK_COMPARE_OP_NEVER;              break;
            }

            return compare;
        }
        static VkImageAspectFlags TextureAspectToVk(TextureAspectFlags flags)
        {
            VkImageAspectFlags aspectFlags = 0;
            if (flags & TEXTURE_ASPECT_COLOR)                aspectFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
            if (flags & TEXTURE_ASPECT_DEPTH)                aspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
            if (flags & TEXTURE_ASPECT_STENCIL)              aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
            return aspectFlags;
        }
        static VkImageSubresourceRange SubresourceToVk(const TextureSubresourceRange& subresource)
        {
            return {
                TextureAspectToVk(subresource.aspect),  // aspectMask
                subresource.baseMipLevel,               // baseMipLevel
                subresource.levelCount,                 // levelCount
                subresource.baseArrayLayer,             // baseArrayLayer
                subresource.layerCount                  // layerCount
            };
        }
        static VkImageViewType TextureViewTypeToVk(TextureViewType textureViewType)
        {
            VkImageViewType type;
            switch (textureViewType) {
            case VIEW_TYPE_1D:              type = VK_IMAGE_VIEW_TYPE_1D;           break;
            case VIEW_TYPE_2D:              type = VK_IMAGE_VIEW_TYPE_2D;           break;
            case VIEW_TYPE_3D:              type = VK_IMAGE_VIEW_TYPE_3D;           break;
            case VIEW_TYPE_CUBE:            type = VK_IMAGE_VIEW_TYPE_CUBE;         break;
            case VIEW_TYPE_1D_ARRAY:        type = VK_IMAGE_VIEW_TYPE_1D_ARRAY;     break;
            case VIEW_TYPE_2D_ARRAY:        type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;     break;
            case VIEW_TYPE_CUBE_ARRAY:      type = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;   break;
            default:                        type = VK_IMAGE_VIEW_TYPE_2D;           break;
            }

            return type;
        }

        static std::string QueueFlagsToString(VkQueueFlags queueFlags)
        {
            std::string str = "";
            if (queueFlags & VK_QUEUE_GRAPHICS_BIT)          str.append("VK_QUEUE_GRAPHICS_BIT ");
            if (queueFlags & VK_QUEUE_COMPUTE_BIT)           str.append("VK_QUEUE_COMPUTE_BIT ");
            if (queueFlags & VK_QUEUE_TRANSFER_BIT)          str.append("VK_QUEUE_TRANSFER_BIT ");
            if (queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)    str.append("VK_QUEUE_SPARSE_BINDING_BIT ");
            if (queueFlags & VK_QUEUE_PROTECTED_BIT)         str.append("VK_QUEUE_PROTECTED_BIT ");
            if (queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)  str.append("VK_QUEUE_VIDEO_DECODE_BIT_KHR ");
            if (queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)   str.append("VK_QUEUE_OPTICAL_FLOW_BIT_NV ");

            return  str;
        }


        inline const char* VKResultToString(VkResult result)
        {
            switch (result)
            {
            case VK_SUCCESS: return "VK_SUCCESS";
            case VK_NOT_READY: return "VK_NOT_READY";
            case VK_TIMEOUT: return "VK_TIMEOUT";
            case VK_EVENT_SET: return "VK_EVENT_SET";
            case VK_EVENT_RESET: return "VK_EVENT_RESET";
            case VK_INCOMPLETE: return "VK_INCOMPLETE";
            case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
            case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
            case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
            case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
            case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
            case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
            case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
            case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
            case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
            case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
            case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
            case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
            case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
            case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
            case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
            case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
            case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
            case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
            case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
            case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
            case VK_ERROR_NOT_PERMITTED_EXT: return "VK_ERROR_NOT_PERMITTED_EXT";
            case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
            case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
            case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
            case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
            case VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
            case VK_PIPELINE_COMPILE_REQUIRED_EXT: return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
            }
            ASSERT(false);
            return nullptr;
        }
        inline void VulkanCheckResult(VkResult result, const char* file, int line)
        {
            if (result != VK_SUCCESS)
            {
                LOG_ERROR("VkResult is '{0}' in {1}:{2}", VKResultToString(result), file, line);
                if (result == VK_ERROR_DEVICE_LOST)
                {
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(3s);

                }
            }
        }
        
        static VkFormat RHIFormatToVkFormat(const RHIFormat& pixelFormat)
        {
            VkFormat format;

            switch (pixelFormat) {
            case FORMAT_UKNOWN:                format = VK_FORMAT_UNDEFINED;               break;

            case FORMAT_R8_SRGB:               format = VK_FORMAT_R8_SRGB;                 break;
            case FORMAT_R8G8_SRGB:             format = VK_FORMAT_R8G8_SRGB;               break;
            case FORMAT_R8G8B8_SRGB:           format = VK_FORMAT_R8G8B8_SRGB;             break;
            case FORMAT_R8G8B8A8_SRGB:         format = VK_FORMAT_R8G8B8A8_SRGB;           break;
            case FORMAT_B8G8R8A8_SRGB:         format = VK_FORMAT_B8G8R8A8_SRGB;           break;

            case FORMAT_R16_SFLOAT:            format = VK_FORMAT_R16_SFLOAT;              break;
            case FORMAT_R16G16_SFLOAT:         format = VK_FORMAT_R16G16_SFLOAT;           break;
            case FORMAT_R16G16B16_SFLOAT:      format = VK_FORMAT_R16G16B16_SFLOAT;        break;
            case FORMAT_R16G16B16A16_SFLOAT:   format = VK_FORMAT_R16G16B16A16_SFLOAT;     break;
            case FORMAT_R32_SFLOAT:            format = VK_FORMAT_R32_SFLOAT;              break;
            case FORMAT_R32G32_SFLOAT:         format = VK_FORMAT_R32G32_SFLOAT;           break;
            case FORMAT_R32G32B32_SFLOAT:      format = VK_FORMAT_R32G32B32_SFLOAT;        break;
            case FORMAT_R32G32B32A32_SFLOAT:   format = VK_FORMAT_R32G32B32A32_SFLOAT;     break;

            case FORMAT_R8_UNORM:             format = VK_FORMAT_R8_UNORM;                break;
            case FORMAT_R8G8_UNORM:           format = VK_FORMAT_R8G8_UNORM;              break;
            case FORMAT_R8G8B8_UNORM:         format = VK_FORMAT_R8G8B8_UNORM;            break;
            case FORMAT_R8G8B8A8_UNORM:       format = VK_FORMAT_R8G8B8A8_UNORM;          break;
            case FORMAT_R16_UNORM:            format = VK_FORMAT_R16_UNORM;               break;
            case FORMAT_R16G16_UNORM:         format = VK_FORMAT_R16G16_UNORM;            break;
            case FORMAT_R16G16B16_UNORM:      format = VK_FORMAT_R16G16B16_UNORM;         break;
            case FORMAT_R16G16B16A16_UNORM:   format = VK_FORMAT_R16G16B16A16_UNORM;      break;

            case FORMAT_R8_SNORM:             format = VK_FORMAT_R8_SNORM;                break;
            case FORMAT_R8G8_SNORM:           format = VK_FORMAT_R8G8_SNORM;              break;
            case FORMAT_R8G8B8_SNORM:         format = VK_FORMAT_R8G8B8_SNORM;            break;
            case FORMAT_R8G8B8A8_SNORM:       format = VK_FORMAT_R8G8B8A8_SNORM;          break;
            case FORMAT_R16_SNORM:            format = VK_FORMAT_R16_SNORM;               break;
            case FORMAT_R16G16_SNORM:         format = VK_FORMAT_R16G16_SNORM;            break;
            case FORMAT_R16G16B16_SNORM:      format = VK_FORMAT_R16G16B16_SNORM;         break;
            case FORMAT_R16G16B16A16_SNORM:   format = VK_FORMAT_R16G16B16A16_SNORM;      break;

            case FORMAT_R8_UINT:              format = VK_FORMAT_R8_UINT;                 break;
            case FORMAT_R8G8_UINT:            format = VK_FORMAT_R8G8_UINT;               break;
            case FORMAT_R8G8B8_UINT:          format = VK_FORMAT_R8G8B8_UINT;             break;
            case FORMAT_R8G8B8A8_UINT:        format = VK_FORMAT_R8G8B8A8_UINT;           break;
            case FORMAT_R16_UINT:             format = VK_FORMAT_R16_UINT;                break;
            case FORMAT_R16G16_UINT:          format = VK_FORMAT_R16G16_UINT;             break;
            case FORMAT_R16G16B16_UINT:       format = VK_FORMAT_R16G16B16_UINT;          break;
            case FORMAT_R16G16B16A16_UINT:    format = VK_FORMAT_R16G16B16A16_UINT;       break;
            case FORMAT_R32_UINT:             format = VK_FORMAT_R32_UINT;                break;
            case FORMAT_R32G32_UINT:          format = VK_FORMAT_R32G32_UINT;             break;
            case FORMAT_R32G32B32_UINT:       format = VK_FORMAT_R32G32B32_UINT;          break;
            case FORMAT_R32G32B32A32_UINT:    format = VK_FORMAT_R32G32B32A32_UINT;       break;

            case FORMAT_R8_SINT:              format = VK_FORMAT_R8_SINT;                 break;
            case FORMAT_R8G8_SINT:            format = VK_FORMAT_R8G8_SINT;               break;
            case FORMAT_R8G8B8_SINT:          format = VK_FORMAT_R8G8B8_SINT;             break;
            case FORMAT_R8G8B8A8_SINT:        format = VK_FORMAT_R8G8B8A8_SINT;           break;
            case FORMAT_R16_SINT:             format = VK_FORMAT_R16_SINT;                break;
            case FORMAT_R16G16_SINT:          format = VK_FORMAT_R16G16_SINT;             break;
            case FORMAT_R16G16B16_SINT:       format = VK_FORMAT_R16G16B16_SINT;          break;
            case FORMAT_R16G16B16A16_SINT:    format = VK_FORMAT_R16G16B16A16_SINT;       break;
            case FORMAT_R32_SINT:             format = VK_FORMAT_R32_SINT;                break;
            case FORMAT_R32G32_SINT:          format = VK_FORMAT_R32G32_SINT;             break;
            case FORMAT_R32G32B32_SINT:       format = VK_FORMAT_R32G32B32_SINT;          break;
            case FORMAT_R32G32B32A32_SINT:    format = VK_FORMAT_R32G32B32A32_SINT;       break;

            case FORMAT_D32_SFLOAT:           format = VK_FORMAT_D32_SFLOAT;              break;
            case FORMAT_D32_SFLOAT_S8_UINT:   format = VK_FORMAT_D32_SFLOAT_S8_UINT;      break;
            case FORMAT_D24_UNORM_S8_UINT:    format = VK_FORMAT_D24_UNORM_S8_UINT;       break;

            default:                          format = VK_FORMAT_UNDEFINED;               break;
            }

            return format;
        }

        static RHIFormat VkFormatToRHIFormat(const VkFormat& vkFormat)
        {
            RHIFormat format;

            switch (vkFormat) {
            case VK_FORMAT_UNDEFINED:            format = FORMAT_UKNOWN;                  break;

            case VK_FORMAT_R8_SRGB:              format = FORMAT_R8_SRGB;                 break;
            case VK_FORMAT_R8G8_SRGB:            format = FORMAT_R8G8_SRGB;               break;
            case VK_FORMAT_R8G8B8_SRGB:          format = FORMAT_R8G8B8_SRGB;             break;
            case VK_FORMAT_R8G8B8A8_SRGB:        format = FORMAT_R8G8B8A8_SRGB;           break;
            case VK_FORMAT_B8G8R8A8_SRGB:        format = FORMAT_B8G8R8A8_SRGB;           break;

            case VK_FORMAT_R16_SFLOAT:           format = FORMAT_R16_SFLOAT;              break;
            case VK_FORMAT_R16G16_SFLOAT:        format = FORMAT_R16G16_SFLOAT;           break;
            case VK_FORMAT_R16G16B16_SFLOAT:     format = FORMAT_R16G16B16_SFLOAT;        break;
            case VK_FORMAT_R16G16B16A16_SFLOAT:  format = FORMAT_R16G16B16A16_SFLOAT;     break;
            case VK_FORMAT_R32_SFLOAT:           format = FORMAT_R32_SFLOAT;              break;
            case VK_FORMAT_R32G32_SFLOAT:        format = FORMAT_R32G32_SFLOAT;           break;
            case VK_FORMAT_R32G32B32_SFLOAT:     format = FORMAT_R32G32B32_SFLOAT;        break;
            case VK_FORMAT_R32G32B32A32_SFLOAT:  format = FORMAT_R32G32B32A32_SFLOAT;     break;

            case VK_FORMAT_R8_UNORM:             format = FORMAT_R8_UNORM;                break;
            case VK_FORMAT_R8G8_UNORM:           format = FORMAT_R8G8_UNORM;              break;
            case VK_FORMAT_R8G8B8_UNORM:         format = FORMAT_R8G8B8_UNORM;            break;
            case VK_FORMAT_R8G8B8A8_UNORM:       format = FORMAT_R8G8B8A8_UNORM;          break;
            case VK_FORMAT_R16_UNORM:            format = FORMAT_R16_UNORM;               break;
            case VK_FORMAT_R16G16_UNORM:         format = FORMAT_R16G16_UNORM;            break;
            case VK_FORMAT_R16G16B16_UNORM:      format = FORMAT_R16G16B16_UNORM;         break;
            case VK_FORMAT_R16G16B16A16_UNORM:   format = FORMAT_R16G16B16A16_UNORM;      break;

            case VK_FORMAT_R8_SNORM:             format = FORMAT_R8_SNORM;                break;
            case VK_FORMAT_R8G8_SNORM:           format = FORMAT_R8G8_SNORM;              break;
            case VK_FORMAT_R8G8B8_SNORM:         format = FORMAT_R8G8B8_SNORM;            break;
            case VK_FORMAT_R8G8B8A8_SNORM:       format = FORMAT_R8G8B8A8_SNORM;          break;
            case VK_FORMAT_R16_SNORM:            format = FORMAT_R16_SNORM;               break;
            case VK_FORMAT_R16G16_SNORM:         format = FORMAT_R16G16_SNORM;            break;
            case VK_FORMAT_R16G16B16_SNORM:      format = FORMAT_R16G16B16_SNORM;         break;
            case VK_FORMAT_R16G16B16A16_SNORM:   format = FORMAT_R16G16B16A16_SNORM;      break;

            case VK_FORMAT_R8_UINT:              format = FORMAT_R8_UINT;                 break;
            case VK_FORMAT_R8G8_UINT:            format = FORMAT_R8G8_UINT;               break;
            case VK_FORMAT_R8G8B8_UINT:          format = FORMAT_R8G8B8_UINT;             break;
            case VK_FORMAT_R8G8B8A8_UINT:        format = FORMAT_R8G8B8A8_UINT;           break;
            case VK_FORMAT_R16_UINT:             format = FORMAT_R16_UINT;                break;
            case VK_FORMAT_R16G16_UINT:          format = FORMAT_R16G16_UINT;             break;
            case VK_FORMAT_R16G16B16_UINT:       format = FORMAT_R16G16B16_UINT;          break;
            case VK_FORMAT_R16G16B16A16_UINT:    format = FORMAT_R16G16B16A16_UINT;       break;
            case VK_FORMAT_R32_UINT:             format = FORMAT_R32_UINT;                break;
            case VK_FORMAT_R32G32_UINT:          format = FORMAT_R32G32_UINT;             break;
            case VK_FORMAT_R32G32B32_UINT:       format = FORMAT_R32G32B32_UINT;          break;
            case VK_FORMAT_R32G32B32A32_UINT:    format = FORMAT_R32G32B32A32_UINT;       break;

            case VK_FORMAT_R8_SINT:              format = FORMAT_R8_SINT;                 break;
            case VK_FORMAT_R8G8_SINT:            format = FORMAT_R8G8_SINT;               break;
            case VK_FORMAT_R8G8B8_SINT:          format = FORMAT_R8G8B8_SINT;             break;
            case VK_FORMAT_R8G8B8A8_SINT:        format = FORMAT_R8G8B8A8_SINT;           break;
            case VK_FORMAT_R16_SINT:             format = FORMAT_R16_SINT;                break;
            case VK_FORMAT_R16G16_SINT:          format = FORMAT_R16G16_SINT;             break;
            case VK_FORMAT_R16G16B16_SINT:       format = FORMAT_R16G16B16_SINT;          break;
            case VK_FORMAT_R16G16B16A16_SINT:    format = FORMAT_R16G16B16A16_SINT;       break;
            case VK_FORMAT_R32_SINT:             format = FORMAT_R32_SINT;                break;
            case VK_FORMAT_R32G32_SINT:          format = FORMAT_R32G32_SINT;             break;
            case VK_FORMAT_R32G32B32_SINT:       format = FORMAT_R32G32B32_SINT;          break;
            case VK_FORMAT_R32G32B32A32_SINT:    format = FORMAT_R32G32B32A32_SINT;       break;

            case VK_FORMAT_D32_SFLOAT:           format = FORMAT_D32_SFLOAT;              break;
            case VK_FORMAT_D32_SFLOAT_S8_UINT:   format = FORMAT_D32_SFLOAT_S8_UINT;      break;
            case VK_FORMAT_D24_UNORM_S8_UINT:    format = FORMAT_D24_UNORM_S8_UINT;       break;

            default:                             format = FORMAT_UKNOWN;                   break;
            }

            return format;
        }
        static VmaMemoryUsage MemoryUsageToVma(MemoryUsage memoryUsage)
        {
            VmaMemoryUsage usage;
            switch (memoryUsage) {
            case MEMORY_USAGE_UNKNOWN:      usage = VMA_MEMORY_USAGE_UNKNOWN;       break;
            case MEMORY_USAGE_GPU_ONLY:     usage = VMA_MEMORY_USAGE_GPU_ONLY;      break;
            case MEMORY_USAGE_CPU_ONLY:     usage = VMA_MEMORY_USAGE_CPU_ONLY;      break;
            case MEMORY_USAGE_CPU_TO_GPU:   usage = VMA_MEMORY_USAGE_CPU_TO_GPU;    break;
            case MEMORY_USAGE_GPU_TO_CPU:   usage = VMA_MEMORY_USAGE_GPU_TO_CPU;    break;
            default:                        usage = VMA_MEMORY_USAGE_UNKNOWN;       break;
            }

            return usage;
        }
        static VkBufferUsageFlags ResourceTypeToImageUsage(ResourceType type)
        {
            VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            if (type & RESOURCE_TYPE_TEXTURE)               usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
            if (type & RESOURCE_TYPE_RW_TEXTURE)            usage |= VK_IMAGE_USAGE_STORAGE_BIT;

            return usage;
        }
        static ResourceType SpvDescriptorTypeToResourceType(SpvReflectDescriptorType descriptorType)
        {
            ResourceType resourceType;
            switch (descriptorType) {
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:                       resourceType = RESOURCE_TYPE_SAMPLER;                   break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:        resourceType = RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER;    break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:                 resourceType = RESOURCE_TYPE_TEXTURE;                   break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:                 resourceType = RESOURCE_TYPE_RW_TEXTURE;                break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:          resourceType = RESOURCE_TYPE_TEXEL_BUFFER;              break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:          resourceType = RESOURCE_TYPE_RW_TEXEL_BUFFER;           break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:                resourceType = RESOURCE_TYPE_UNIFORM_BUFFER;            break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:                resourceType = RESOURCE_TYPE_RW_BUFFER;                 break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:        resourceType = RESOURCE_TYPE_UNIFORM_BUFFER;            break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:        resourceType = RESOURCE_TYPE_RW_BUFFER;                 break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:    resourceType = RESOURCE_TYPE_RAY_TRACING;               break;
            default:                                                        LOG_ERROR("Unsupported reflect descriptor type!");
            }

            return resourceType;
        }
        static ShaderFrequency SpvShaderStageToFrequency(SpvReflectShaderStageFlagBits spvShaderStage)
        {
            ShaderFrequency frequency;
            switch (spvShaderStage) {
            case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:               frequency = SHADER_FREQUENCY_VERTEX;         break;
            case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:             frequency = SHADER_FREQUENCY_GEOMETRY;       break;
            case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:             frequency = SHADER_FREQUENCY_FRAGMENT;       break;
            case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:              frequency = SHADER_FREQUENCY_COMPUTE;        break;
            case SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV:              frequency = SHADER_FREQUENCY_MESH;           break;
            case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR:           frequency = SHADER_FREQUENCY_RAY_GEN;        break;
            case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR:          frequency = SHADER_FREQUENCY_ANY_HIT;        break;
            case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:      frequency = SHADER_FREQUENCY_CLOSEST_HIT;    break;
            case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR:             frequency = SHADER_FREQUENCY_RAY_MISS;       break;
            case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR:     frequency = SHADER_FREQUENCY_INTERSECTION;   break;
            default:                                                frequency = SHADER_FREQUENCY_ALL;            break;
            }

            return frequency;
        }
        static RHIFormat SpvFormatToRHIFormat(const SpvReflectFormat& spvFormat)
        {
            RHIFormat format;

            switch (spvFormat) {
            case SPV_REFLECT_FORMAT_UNDEFINED:              format = FORMAT_UKNOWN;                 break;
            case SPV_REFLECT_FORMAT_R16_UINT:               format = FORMAT_R16_UINT;               break;
            case SPV_REFLECT_FORMAT_R16_SINT:               format = FORMAT_R16_SINT;               break;
            case SPV_REFLECT_FORMAT_R16_SFLOAT:             format = FORMAT_R16_SFLOAT;             break;
            case SPV_REFLECT_FORMAT_R16G16_UINT:            format = FORMAT_R16G16_UINT;            break;
            case SPV_REFLECT_FORMAT_R16G16_SINT:            format = FORMAT_R16G16_SINT;            break;
            case SPV_REFLECT_FORMAT_R16G16_SFLOAT:          format = FORMAT_R16G16_SFLOAT;          break;
            case SPV_REFLECT_FORMAT_R16G16B16_UINT:         format = FORMAT_R16G16B16_UINT;         break;
            case SPV_REFLECT_FORMAT_R16G16B16_SINT:         format = FORMAT_R16G16B16_SINT;         break;
            case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT:       format = FORMAT_R16G16B16_SFLOAT;       break;
            case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:      format = FORMAT_R16G16B16A16_UINT;      break;
            case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:      format = FORMAT_R16G16B16A16_SINT;      break;
            case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:    format = FORMAT_R16G16B16A16_SFLOAT;    break;
            case SPV_REFLECT_FORMAT_R32_UINT:               format = FORMAT_R32_UINT;               break;
            case SPV_REFLECT_FORMAT_R32_SINT:               format = FORMAT_R32_SINT;               break;
            case SPV_REFLECT_FORMAT_R32_SFLOAT:             format = FORMAT_R32_SFLOAT;             break;
            case SPV_REFLECT_FORMAT_R32G32_UINT:            format = FORMAT_R32G32_UINT;            break;
            case SPV_REFLECT_FORMAT_R32G32_SINT:            format = FORMAT_R32G32_SINT;            break;
            case SPV_REFLECT_FORMAT_R32G32_SFLOAT:          format = FORMAT_R32G32_SFLOAT;          break;
            case SPV_REFLECT_FORMAT_R32G32B32_UINT:         format = FORMAT_R32G32B32_UINT;         break;
            case SPV_REFLECT_FORMAT_R32G32B32_SINT:         format = FORMAT_R32G32B32_SINT;         break;
            case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:       format = FORMAT_R32G32B32_SFLOAT;       break;
            case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:      format = FORMAT_R32G32B32A32_UINT;      break;
            case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:      format = FORMAT_R32G32B32A32_SINT;      break;
            case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:    format = FORMAT_R32G32B32A32_SFLOAT;    break;
            default:                                        LOG_ERROR("Unsupported reflect format type!");
            }

            return format;
        }

        static VkBufferUsageFlags ResourceTypeToBufferUsage(ResourceType type)
        {
            VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            if (type & RESOURCE_TYPE_UNIFORM_BUFFER)        usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            if (type & RESOURCE_TYPE_RW_BUFFER)             usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            if (type & RESOURCE_TYPE_BUFFER)                usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            // if (type & RESOURCE_TYPE_INDEX_BUFFER)          usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
            if (type & RESOURCE_TYPE_INDEX_BUFFER)          usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            // if (type & RESOURCE_TYPE_VERTEX_BUFFER)         usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
            if (type & RESOURCE_TYPE_VERTEX_BUFFER)         usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            if (type & RESOURCE_TYPE_INDIRECT_BUFFER)       usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            // if (type & RESOURCE_TYPE_RAY_TRACING)           usage |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

            return usage;
        }
        static VkShaderStageFlags ShaderFrequencyToVkStageFlags(ShaderFrequency frequency)
        {
            VkShaderStageFlags stageFlags = 0;
            if (frequency & SHADER_FREQUENCY_COMPUTE)        stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
            if (frequency & SHADER_FREQUENCY_VERTEX)         stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
            if (frequency & SHADER_FREQUENCY_FRAGMENT)       stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
            if (frequency & SHADER_FREQUENCY_GEOMETRY)       stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;
            if (frequency & SHADER_FREQUENCY_RAY_GEN)        stageFlags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            if (frequency & SHADER_FREQUENCY_CLOSEST_HIT)    stageFlags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            if (frequency & SHADER_FREQUENCY_RAY_MISS)       stageFlags |= VK_SHADER_STAGE_MISS_BIT_KHR;
            if (frequency & SHADER_FREQUENCY_INTERSECTION)   stageFlags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            if (frequency & SHADER_FREQUENCY_ANY_HIT)        stageFlags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            if (frequency & SHADER_FREQUENCY_MESH)           stageFlags |= VK_SHADER_STAGE_MESH_BIT_EXT;
            return stageFlags;
        }
        static VkDescriptorType ResourceTypeToVk(ResourceType resourceType)
        {
            VkDescriptorType descriptorType;
            switch (resourceType) {
            case RESOURCE_TYPE_SAMPLER:                 descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;                        break;
            case RESOURCE_TYPE_TEXTURE_CUBE:            descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;                  break;
            case RESOURCE_TYPE_TEXTURE:                 descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;                  break;
            case RESOURCE_TYPE_RW_TEXTURE:              descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;                  break;
            case RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER:  descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;         break;
            case RESOURCE_TYPE_BUFFER:                  descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;                 break;
            case RESOURCE_TYPE_RW_BUFFER:               descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;                 break;
            case RESOURCE_TYPE_UNIFORM_BUFFER:          descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;                 break;
            case RESOURCE_TYPE_TEXEL_BUFFER:            descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;           break;
            case RESOURCE_TYPE_RW_TEXEL_BUFFER:         descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;           break;
            case RESOURCE_TYPE_RAY_TRACING:             descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;     break;
            default:                                    LOG_ERROR("Unsupported resource type!");
            }
            return descriptorType;
        }
        static VkImageLayout ResourceTypeToImageLayout(ResourceType resourceType)
        {
            VkImageLayout imageLayout;
            switch (resourceType) {

            case RESOURCE_TYPE_TEXTURE:                 imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;         break;
            case RESOURCE_TYPE_TEXTURE_CUBE:            imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;         break;
            case RESOURCE_TYPE_RW_TEXTURE:              imageLayout = VK_IMAGE_LAYOUT_GENERAL;                          break;
            case RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER:  imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;         break;
            default:                                    LOG_ERROR("Unsupported resource type!");
            }
            return imageLayout;
        }
        static VkAttachmentLoadOp AttachmentLoadOpToVk(AttachmentLoadOp loadOp)
        {
            VkAttachmentLoadOp op;
            switch (loadOp) {
            case ATTACHMENT_LOAD_OP_LOAD:           op = VK_ATTACHMENT_LOAD_OP_LOAD;        break;
            case ATTACHMENT_LOAD_OP_CLEAR:          op = VK_ATTACHMENT_LOAD_OP_CLEAR;       break;
            case ATTACHMENT_LOAD_OP_DONT_CARE:      op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   break;
            default:                                LOG_ERROR("Unsupported attachment load op!");
            }

            return op;
        }

        static VkAttachmentStoreOp AttachmentStoreOpToVk(AttachmentStoreOp storeOp)
        {
            VkAttachmentStoreOp op;
            switch (storeOp) {
            case ATTACHMENT_STORE_OP_STORE:          op = VK_ATTACHMENT_STORE_OP_STORE;       break;
            case ATTACHMENT_STORE_OP_DONT_CARE:      op = VK_ATTACHMENT_STORE_OP_DONT_CARE;   break;
            default:                                 LOG_ERROR("Unsupported attachment store op!");
            }

            return op;
        }

        static VkPushConstantRange GetPushConstantInfo(const PushConstantInfo& pushConstant)
        {
            VkPushConstantRange pushConstantRange{};
            pushConstantRange.stageFlags = ShaderFrequencyToVkStageFlags(pushConstant.frequency);
            pushConstantRange.offset = 0;
            pushConstantRange.size = pushConstant.size;

            return pushConstantRange;
        }
        static VkPipelineLayout CreatePipelineLayout(VkDevice device,
            const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
            const std::vector<VkPushConstantRange>& pushConstantRanges)
        {
            VkPipelineLayout layout;

            // 管道布局信息
            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
            pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
            pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

            if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS)
            {
                LOG_ERROR("Failed to create pipeline layout!");
            }
            return layout;
        }
        typedef struct VulkanRenderPassAttachments
        {
            std::vector<VkAttachmentDescription> colorAttachments;
            VkAttachmentDescription depthStencilAttachment = {};

        } VulkanRenderPassAttachments;


        static VkPrimitiveTopology PrimitiveTypeToVk(PrimitiveType primitiveType)
        {
            VkPrimitiveTopology topology;
            switch (primitiveType) {
            case PRIMITIVE_TYPE_TRIANGLE_LIST:      topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;     break;
            case PRIMITIVE_TYPE_TRIANGLE_STRIP:     topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;    break;
            case PRIMITIVE_TYPE_LINE_LIST:          topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;         break;
            case PRIMITIVE_TYPE_POINT_LIST:         topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;        break;
            default:                                LOG_ERROR("Unsupported primitive type!");
            }

            return topology;
        }
        static VkPolygonMode FillModeToVk(RasterizerFillMode fillMode)
        {
            VkPolygonMode mode;
            switch (fillMode) {
            case FILL_MODE_POINT:           mode = VK_POLYGON_MODE_POINT;   break;
            case FILL_MODE_WIREFRAME:       mode = VK_POLYGON_MODE_LINE;    break;
            case FILL_MODE_SOLID:           mode = VK_POLYGON_MODE_FILL;    break;
            default:                        LOG_ERROR("Unsupported fill mode!");
            }

            return mode;
        }

        static VkCullModeFlags CullModeToVk(RasterizerCullMode cullMode)
        {
            VkCullModeFlags mode;
            switch (cullMode) {
            case CULL_MODE_NONE:        mode = VK_CULL_MODE_NONE;           break;
            case CULL_MODE_FRONT:       mode = VK_CULL_MODE_FRONT_BIT;      break;  // 逆时针为正面
            case CULL_MODE_BACK:        mode = VK_CULL_MODE_BACK_BIT;       break;
            default:                    LOG_ERROR("Unsupported cull mode!");
            }

            return mode;
        }

        static VkBlendOp BlendOpToVk(BlendOp blendOp)
        {
            VkBlendOp op;
            switch (blendOp) {
            case BLEND_OP_ADD:                  op = VK_BLEND_OP_ADD;                   break;
            case BLEND_OP_SUBTRACT:             op = VK_BLEND_OP_SUBTRACT;              break;
            case BLEND_OP_REVERSE_SUBTRACT:     op = VK_BLEND_OP_REVERSE_SUBTRACT;      break;
            case BLEND_OP_MIN:                  op = VK_BLEND_OP_MIN;                   break;
            case BLEND_OP_MAX:                  op = VK_BLEND_OP_MAX;                   break;
            default:                            LOG_ERROR("Unsupported blend op!");
            }

            return op;
        }
        static VkBlendFactor BlendFactorToVk(BlendFactor blendFactor)
        {
            VkBlendFactor factor;
            switch (blendFactor) {
            case BLEND_FACTOR_ZERO:                         factor = VK_BLEND_FACTOR_ZERO;                          break;
            case BLEND_FACTOR_ONE:                          factor = VK_BLEND_FACTOR_ONE;                           break;
            case BLEND_FACTOR_SRC_COLOR:                    factor = VK_BLEND_FACTOR_SRC_COLOR;                     break;
            case BLEND_FACTOR_ONE_MINUS_SRC_COLOR:          factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;           break;
            case BLEND_FACTOR_DST_COLOR:                    factor = VK_BLEND_FACTOR_DST_COLOR;                     break;
            case BLEND_FACTOR_ONE_MINUS_DST_COLOR:          factor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;           break;
            case BLEND_FACTOR_SRC_ALPHA:                    factor = VK_BLEND_FACTOR_SRC_ALPHA;                     break;
            case BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:          factor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;           break;
            case BLEND_FACTOR_DST_ALPHA:                    factor = VK_BLEND_FACTOR_DST_ALPHA;                     break;
            case BLEND_FACTOR_ONE_MINUS_DST_ALPHA:          factor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;           break;
            case BLEND_FACTOR_SRC_ALPHA_SATURATE:           factor = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;            break;
            case BLEND_FACTOR_CONSTANT_COLOR:               factor = VK_BLEND_FACTOR_CONSTANT_COLOR;                break;
            case BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:     factor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;      break;
            default:                                        LOG_ERROR("Unsupported blend factor!");
            }

            return factor;
        }
        static VkExtent2D ExtentToVk(const Extent2D& extent)
        {
            return { extent.width, extent.height };
        }
        static VkColorComponentFlags ColorWriteMaskToVk(ColorWriteMasks mask)
        {
            VkColorComponentFlags flags = 0;
            if (mask & COLOR_MASK_RED)       flags |= VK_COLOR_COMPONENT_R_BIT;
            if (mask & COLOR_MASK_GREEN)     flags |= VK_COLOR_COMPONENT_G_BIT;
            if (mask & COLOR_MASK_BLUE)      flags |= VK_COLOR_COMPONENT_B_BIT;
            if (mask & COLOR_MASK_ALPHA)     flags |= VK_COLOR_COMPONENT_A_BIT;
            return flags;
        }
        static VkImageSubresourceLayers SubresourceToVk(const TextureSubresourceLayers& subresource)
        {
            return {
                TextureAspectToVk(subresource.aspect),
                subresource.mipLevel,
                subresource.baseArrayLayer,
                subresource.layerCount
            };
        }

        static VkExtent3D ExtentToVk(const Extent3D& extent)
        {
            return { extent.width, extent.height, extent.depth};
        }
	}

}