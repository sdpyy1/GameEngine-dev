#include "hzpch.h"
#include "VulkanRHI.h"
#include "VulkanRHICache.h"
namespace GameEngine {
    VkRenderPassCache::CachedRenderPass VkRenderPassCache::Allocate(const VulkanUtil::VulkanRenderPassAttachments& info)
    {
        VkRenderPassCache::CachedRenderPass ret;

        auto iter = cachedPasses.find(info);
        if (iter != cachedPasses.end())
        {
            return iter->second;
        }
#ifdef RDG_DEBUG
        LOG_WARN("VkRenderPass not found in cache, creating new.");
#endif
        ret.pass = VULKAN_RHI->CreateVkRenderPass(info);
        cachedPasses[info] = ret;

        return ret;
    }

    void VkRenderPassCache::Clear()
    {
        for (auto iter : cachedPasses)
        {
            // vkDestroyRenderPass(VULKAN_DEVICE, iter.second.pass, nullptr); // TODO：关闭APP时这里会报错
        }
        cachedPasses.clear();
    }

    VkFramebufferCache::CachedFramebuffer VkFramebufferCache::Allocate(const VkFramebufferCreateInfo& info)
    {
        VkFramebufferCache::CachedFramebuffer ret;

        auto iter = cachedFramebuffers.find(info);
        if (iter != cachedFramebuffers.end())
        {
            // LOG_DEBUG("VkFramebuffer found in cache.");
            return iter->second;
        }
#ifdef RDG_DEBUG
        LOG_WARN("VkFramebuffer not found in cache, creating new.");
#endif
        ret.frameBuffer = VULKAN_RHI->CreateVkFramebuffer(info);

        cachedFramebuffers[info] = ret;

        return ret;
    }

    void VkFramebufferCache::Clear()
    {
        if (cachedFramebuffers.empty()) return;
        for (auto iter : cachedFramebuffers)
        {
            // vkDestroyFramebuffer(VULKAN_DEVICE, iter.second.frameBuffer, nullptr); //TODO:关闭APP时这里会报错
        }
        //cachedFramebuffers.clear();
    }
}