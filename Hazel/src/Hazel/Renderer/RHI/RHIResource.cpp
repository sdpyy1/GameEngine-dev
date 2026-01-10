#include "hzpch.h"
#include "RHIResource.h"
#include "RHI.h"
#include "RHICommandList.h"
namespace GameEngine {

	Extent3D RHITexture::MipExtent(uint32_t mipLevel)
	{
        if (mipLevel > info.mipLevels) LOG_WARN("Mip level is greater than texture`s max mip!");

        Extent3D extent = info.extent;
        extent.width = std::max((uint32_t)1, extent.width >> mipLevel);
        extent.height = std::max((uint32_t)1, extent.height >> mipLevel);
        extent.depth = std::max((uint32_t)1, extent.depth >> mipLevel);

        return extent;
	}

    RHICommandListRef RHICommandPool::CreateCommandList(bool byPass)
    {
        CommandListInfo info;
        info.pool = shared_from_this();
        info.context = DynamicRHI::Get()->CreateCommandContext(shared_from_this());
        info.byPass = byPass;
        RHICommandListRef commandList = std::make_shared<RHICommandList>(info);
        return commandList;
    }

    void RHIResource::printRawHandle()
    {
        // 转成 uint64_t 更通用：可打印指针、Vulkan 句柄、D3D12 句柄
        uint64_t handle = reinterpret_cast<uint64_t>(RawHandle());
        LOG_INFO("RawHandle({}) = 0x{:x}", RHIResourceTypeToString(GetType()),handle);
    }

}