#include "hzpch.h"
#include "RHIBase.h"
#include "RHIResource.h"

namespace GameEngine {

	RHIRootSignatureInfo& RHIRootSignatureInfo::AddEntry(const ShaderResourceEntry& entry)
	{
        // 合并已有的 
        for (ShaderResourceEntry& oldEntry : entries)
        {
            if (oldEntry.set == entry.set &&
                oldEntry.binding == entry.binding)
            {
                if (oldEntry.type != entry.type)
                {
                    LOG_ERROR("Conflict shader resource entry!");
                    return *this;
                }
                else
                {
                    if ((oldEntry.size == 0 || entry.size == 0) && (oldEntry.size == 1 || entry.size == 1))  oldEntry.size = 0;
                    else oldEntry.size = std::max(oldEntry.size, entry.size);

                    oldEntry.frequency |= entry.frequency;
                    return *this;
                }
            }
        }

        // 新加入
        entries.push_back(entry);
        return *this;
	}


    RHIRootSignatureInfo& RHIRootSignatureInfo::AddEntry(const RHIRootSignatureInfo& other)
    {
        for (auto& entry : other.entries) AddEntry(entry);
        return *this;
    }

    RHIRootSignatureInfo& RHIRootSignatureInfo::AddEntryFromReflect(RHIShaderRef shader)
    {
        ShaderReflectInfo reflectInfo = shader->GetReflectInfo();

        for (ShaderResourceEntry& reflectEntry : reflectInfo.resources) AddEntry(reflectEntry);
        return *this;
    }

	std::string RHIResourceStateToString(RHIResourceState state)
	{
		switch (state)
		{
		case RESOURCE_STATE_UNDEFINED:              return "RESOURCE_STATE_UNDEFINED";
		case RESOURCE_STATE_COMMON:                 return "RESOURCE_STATE_COMMON";
		case RESOURCE_STATE_TRANSFER_SRC:           return "RESOURCE_STATE_TRANSFER_SRC";
		case RESOURCE_STATE_TRANSFER_DST:           return "RESOURCE_STATE_TRANSFER_DST";
		case RESOURCE_STATE_VERTEX_BUFFER:          return "RESOURCE_STATE_VERTEX_BUFFER";
		case RESOURCE_STATE_INDEX_BUFFER:           return "RESOURCE_STATE_INDEX_BUFFER";
		case RESOURCE_STATE_COLOR_ATTACHMENT:       return "RESOURCE_STATE_COLOR_ATTACHMENT";
		case RESOURCE_STATE_DEPTH_STENCIL_ATTACHMENT: return "RESOURCE_STATE_DEPTH_STENCIL_ATTACHMENT";
		case RESOURCE_STATE_UNORDERED_ACCESS:       return "RESOURCE_STATE_UNORDERED_ACCESS";
		case RESOURCE_STATE_SHADER_RESOURCE:        return "RESOURCE_STATE_SHADER_RESOURCE";
		case RESOURCE_STATE_INDIRECT_ARGUMENT:      return "RESOURCE_STATE_INDIRECT_ARGUMENT";
		case RESOURCE_STATE_PRESENT:                return "RESOURCE_STATE_PRESENT";
		case RESOURCE_STATE_ACCELERATION_STRUCTURE: return "RESOURCE_STATE_ACCELERATION_STRUCTURE";
		case RESOURCE_STATE_MAX_ENUM:               return "RESOURCE_STATE_MAX_ENUM";
		default:
			throw std::invalid_argument("Invalid RHIResourceState value: " + std::to_string(static_cast<uint32_t>(state)));
		}
	}

	const char* RHIResourceTypeToString(RHIResourceType type)
	{
		switch (type)
		{
		case RHI_BUFFER:                                 return "RHI_BUFFER";
		case RHI_TEXTURE:                                return "RHI_TEXTURE";
		case RHI_TEXTURE_VIEW:                           return "RHI_TEXTURE_VIEW";
		case RHI_SAMPLER:                                return "RHI_SAMPLER";
		case RHI_SHADER:                                 return "RHI_SHADER";
		case RHI_SHADER_BINDING_TABLE:                   return "RHI_SHADER_BINDING_TABLE";
		case RHI_TOP_LEVEL_ACCELERATION_STRUCTURE:       return "RHI_TLAS";
		case RHI_BOTTOM_LEVEL_ACCELERATION_STRUCTURE:    return "RHI_BLAS";

		case RHI_ROOT_SIGNATURE:                         return "RHI_ROOT_SIGNATURE";
		case RHI_DESCRIPTOR_SET:                         return "RHI_DESCRIPTOR_SET";

		case RHI_RENDER_PASS:                            return "RHI_RENDER_PASS";
		case RHI_GRAPHICS_PIPELINE:                      return "RHI_GRAPHICS_PIPELINE";
		case RHI_COMPUTE_PIPELINE:                       return "RHI_COMPUTE_PIPELINE";
		case RHI_RAY_TRACING_PIPELINE:                   return "RHI_RT_PIPELINE";

		case RHI_QUEUE:                                  return "RHI_QUEUE";
		case RHI_SURFACE:                                return "RHI_SURFACE";
		case RHI_SWAPCHAIN:                              return "RHI_SWAPCHAIN";
		case RHI_COMMAND_POOL:                           return "RHI_COMMAND_POOL";
		case RHI_COMMAND_CONTEXT:                        return "RHI_COMMAND_CONTEXT";
		case RHI_COMMAND_CONTEXT_IMMEDIATE:              return "RHI_COMMAND_CONTEXT_IMMEDIATE";
		case RHI_FENCE:                                  return "RHI_FENCE";
		case RHI_SEMAPHORE:                              return "RHI_SEMAPHORE";

		default:                                         return "UNKNOWN_RHI_RESOURCE_TYPE";
		}
	}



	

}