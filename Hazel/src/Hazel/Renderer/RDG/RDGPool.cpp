#include "hzpch.h"
#include "RDGPool.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderManager.h"

namespace GameEngine {
    RDGBufferPool::PooledBuffer RDGBufferPool::Allocate(const RHIBufferInfo& info)
    {
        RDGBufferPool::PooledBuffer ret;

        auto& buffers = pooledBuffers[info];
        for (auto iter = buffers.begin(); iter != buffers.end(); iter++)
        {
            if (iter->buffer->GetInfo().size >= info.size)  // 如果缓存的Buffer尺寸大于需要，就直接用这块buffer
            {
                ret = *iter;
                buffers.erase(iter);
                pooledSize--;
                return ret;
            }
        }
#ifdef RDG_DEBUG
        LOG_TRACE("RHIBuffer not found in cache, creating new.");
#endif
        ret.buffer = APP_DYNAMICRHI->CreateBuffer(info);
        ret.state = RESOURCE_STATE_UNDEFINED;
        allocatedSize++;
        return ret;
    }

    void RDGBufferPool::Release(const RDGBufferPool::PooledBuffer& pooledBuffer)
    {
        pooledBuffers[pooledBuffer.buffer->GetInfo()].push_back(pooledBuffer);
        pooledSize++;
    }

    RDGTexturePool::PooledTexture RDGTexturePool::Allocate(const RHITextureInfo& info)
    {
        RDGTexturePool::PooledTexture ret;
        RHITextureInfo tempInfo = info;
        if (tempInfo.mipLevels == 0) tempInfo.mipLevels = tempInfo.extent.MipSize(); // auto mip

        auto& textures = pooledTextures[{tempInfo}];
        for (auto iter = textures.begin(); iter != textures.end(); iter++)   // TODO:这个For总是返回第一项
        {
            ret = *iter;
            textures.erase(iter);
            pooledSize--;
            return ret;
        }
#ifdef RDG_DEBUG
        LOG_TRACE("RHITexture not found in cache, creating new.");
#endif
        ret.texture = APP_DYNAMICRHI->CreateTexture(tempInfo);   // 在释放资源时才会把texture放入池中

        ret.state = RESOURCE_STATE_UNDEFINED; // RHI接口创建的texture的state是UNDEFINED



        allocatedSize++;

        return ret;
    }

    void RDGTexturePool::Release(const RDGTexturePool::PooledTexture& pooledTexture)
    {
        pooledTextures[{pooledTexture.texture->GetInfo()}].push_back(pooledTexture);
        pooledSize++;
    }

    RDGTextureViewPool::PooledTextureView RDGTextureViewPool::Allocate(const RHITextureViewInfo& info)
    {
        RHITextureViewInfo actualInfo = info;   // RHI计算的时候也会用默认subresource替换，需要避免分配和返回的info不一致
        if (actualInfo.subresource.aspect == TEXTURE_ASPECT_NONE)  actualInfo.subresource = actualInfo.texture->GetDefaultSubresourceRange();

        RDGTextureViewPool::PooledTextureView ret;

        auto& textureViews = pooledTextureViews[actualInfo];
        for (auto iter = textureViews.begin(); iter != textureViews.end(); iter++)
        {
            ret = *iter;
            textureViews.erase(iter);
            pooledSize--;
            return ret;
        }
#ifdef RDG_DEBUG
        LOG_TRACE("RHITextureView not found in cache, creating new.");
#endif
        ret.textureView = APP_DYNAMICRHI->CreateTextureView(actualInfo);
        allocatedSize++;

        return ret;
    }

    void RDGTextureViewPool::Release(const RDGTextureViewPool::PooledTextureView& pooledTextureView)
    {
        pooledTextureViews[pooledTextureView.textureView->GetInfo()].push_back(pooledTextureView);
        pooledSize++;
    }


    RDGDescriptorSetPool::PooledDescriptor RDGDescriptorSetPool::Allocate(const RHIRootSignatureRef& rootSignature, uint32_t set)
    {
        RDGDescriptorSetPool::PooledDescriptor ret;

        auto& descriptors = pooledDescriptors[{rootSignature->GetInfo(), set}];
        for (auto iter = descriptors.begin(); iter != descriptors.end(); iter++)
        {
            ret = *iter;
            descriptors.erase(iter);
            pooledSize--;
            return ret;
        }
#ifdef RDG_DEBUG
        LOG_TRACE("RHIDescriptorSet not found in cache, creating new.");
#endif
        ret.descriptor = rootSignature->CreateDescriptorSet(set);
        allocatedSize++;

        return ret;
    }

    void RDGDescriptorSetPool::Release(const RDGDescriptorSetPool::PooledDescriptor& pooledDescriptor, const RHIRootSignatureRef& rootSignature, uint32_t set)
    {
        pooledDescriptors[{rootSignature->GetInfo(), set}].push_back(pooledDescriptor);
        pooledSize++;
    }
}