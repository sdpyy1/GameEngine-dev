#include "hzpch.h"
#include "PipelineCache.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
namespace GameEngine
{
#include <iomanip>

    void print_memory(const void* ptr, size_t size) {
        const unsigned char* p = static_cast<const unsigned char*>(ptr);
        for (size_t i = 0; i < size; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(p[i]);
            if ((i + 1) % 16 == 0) std::cout << std::endl;
            else std::cout << " ";
        }
        std::cout << std::dec << std::endl;
    }

    GraphicsPipelineCache::CachedPipeline GraphicsPipelineCache::Allocate(const RHIGraphicsPipelineInfo& info)
    {
        GraphicsPipelineCache::CachedPipeline ret;
        // print_memory(&normalizedInfo, sizeof(RHIGraphicsPipelineInfo));
        auto iter = cachedPipelines.find(info);
        if (iter != cachedPipelines.end())
        {
            // LOG_TRACE("RHIGraphicsPipelineInfo found in cache.");
            return iter->second;
        }

        if (!IsValid(info))
        {
            // LOG_TRACE("RHIGraphicsPipelineInfo is not valid!");
            return { nullptr };
        }
        LOG_WARN("RHIGraphicsPipeline not found in cache, creating new.");

        ret = { APP_DYNAMICRHI->CreateGraphicsPipeline(info)};
        cachedPipelines[info] = ret;
        return ret;
    }

    bool GraphicsPipelineCache::IsValid(RHIGraphicsPipelineInfo info)
    {
        if (!info.vertexShader || !info.fragmentShader || !info.rootSignature) return false;

        for (auto& input : info.vertexShader->GetReflectInfo().inputVariables)
        {
            if (input != FORMAT_UKNOWN) return false;    // 目前使用的管线里全部bindless，所以顶点输入一定为空
        }
        if (info.geometryShader)
        {
            if (info.vertexShader->GetReflectInfo().outputVariables != info.geometryShader->GetReflectInfo().inputVariables) return false;
            if (info.geometryShader->GetReflectInfo().outputVariables != info.fragmentShader->GetReflectInfo().inputVariables) return false;
        }
        else
        {
            if (info.vertexShader->GetReflectInfo().outputVariables != info.fragmentShader->GetReflectInfo().inputVariables) return false;
        }
        for (uint32_t i = 0; i < std::min(info.colorAttachmentFormats.size(), info.fragmentShader->GetReflectInfo().outputVariables.size()); i++)
        {
            if (FormatChanelCounts(info.colorAttachmentFormats[i]) !=
                FormatChanelCounts(info.fragmentShader->GetReflectInfo().outputVariables[i])) return false;    // 输出通道数一致
        }

        // TODO 描述符等信息的检测
        return true;
    }
}