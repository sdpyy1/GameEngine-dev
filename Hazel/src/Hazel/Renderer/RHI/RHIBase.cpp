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
                    else oldEntry.size = std::max(oldEntry.size, entry.size);        //反射得到的bindless数组的数量是0，把0当最大值吧

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




	

}