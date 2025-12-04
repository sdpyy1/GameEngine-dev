#pragma once
#include "RHIBase.h"
namespace GameEngine
{
#if ENABLE_DEBUG_MODE
    #define COMMANDLIST_DEBUG_OUTPUT() do {  \
            printf("RHICommandList[%d]: %s\n", currentCommandIndex, __FUNCTION__);  \
            currentCommandIndex++;  \
        } while(0)  
    #define COMMANDLIST_DEBUG_RESET_INDEX() do {  \
            currentCommandIndex = 0;  \
        } while(0) 
#else 
    #define COMMANDLIST_DEBUG_OUTPUT() do {} while(0)
    #define COMMANDLIST_DEBUG_RESET_INDEX() do {} while(0)
#endif



    /*UE里的Command对象传入的是CommandList对象，Execute时获取CommandList的GetContext的具体执行*/
    /*RHICommand给每个命令套了一个壳子，其实本质还是调用context提供的方法，但是他会组装一个命令需要的参数*/
    typedef struct RHICommand
    {
        RHICommand() = default;
        virtual ~RHICommand() = default;

        virtual void Execute(RHICommandContextRef context) = 0;
    } RHICommand;
    typedef struct RHICommandImmediate
    {
        RHICommandImmediate() = default;
        virtual ~RHICommandImmediate() = default;

        virtual void Execute(RHICommandContextImmediateRef context) = 0;
    } RHICommandImmediate;
    // 各种Command的封装
    struct RHICommandBeginCommand : public RHICommand
    {
        RHICommandBeginCommand() {}

        virtual void Execute(RHICommandContextRef context) override final;
    };
    struct RHICommandEndCommand : public RHICommand
    {
        RHICommandEndCommand() {}

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandBufferBarrier : public RHICommand
    {
        RHIBufferBarrier barrier;

        RHICommandBufferBarrier(const RHIBufferBarrier& barrier)
            : barrier(barrier)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };
    struct RHICommandTextureBarrier : public RHICommand
    {
        RHITextureBarrier barrier;

        RHICommandTextureBarrier(const RHITextureBarrier& barrier)
            : barrier(barrier)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };
    struct RHICommandGenerateMips : public RHICommand
    {
        RHITextureRef src;

        RHICommandGenerateMips(RHITextureRef src)
            : src(src)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };
    struct RHICommandBeginRenderPass : public RHICommand
    {
        RHIRenderPassRef renderPass;

        RHICommandBeginRenderPass(RHIRenderPassRef renderPass)
            : renderPass(renderPass)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };
    struct RHICommandEndRenderPass : public RHICommand
    {
        RHICommandEndRenderPass() {}

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandCopyTexture : public RHICommand
    {
        RHITextureRef src;
        TextureSubresourceLayers srcSubresource;
        RHITextureRef dst;
        TextureSubresourceLayers dstSubresource;

        RHICommandCopyTexture(RHITextureRef src, TextureSubresourceLayers srcSubresource, RHITextureRef dst, TextureSubresourceLayers dstSubresource)
            : src(src)
            , srcSubresource(srcSubresource)
            , dst(dst)
            , dstSubresource(dstSubresource)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    // 各种CommandImmediate的封装
    struct RHICommandImmediateGenerateMips : public RHICommandImmediate
    {
        RHITextureRef src;

        RHICommandImmediateGenerateMips(RHITextureRef src)
            : src(src)
        {
        }
        virtual void Execute(RHICommandContextImmediateRef context) override final;
    };

    struct RHICommandImmediateTextureBarrier : public RHICommandImmediate
    {
        RHITextureBarrier barrier;

        RHICommandImmediateTextureBarrier(const RHITextureBarrier& barrier)
            : barrier(barrier)
        {
        }

        virtual void Execute(RHICommandContextImmediateRef context) override final;
    };
    struct RHICommandSetViewport : public RHICommand
    {
        Offset2D min;
        Offset2D max;

        RHICommandSetViewport(Offset2D min, Offset2D max)
            : min(min)
            , max(max)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandSetScissor : public RHICommand
    {
        Offset2D min;
        Offset2D max;

        RHICommandSetScissor(Offset2D min, Offset2D max)
            : min(min)
            , max(max)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandSetDepthBias : public RHICommand
    {
        float constantBias;
        float slopeBias;
        float clampBias;

        RHICommandSetDepthBias(float constantBias, float slopeBias, float clampBias)
            : constantBias(constantBias)
            , slopeBias(slopeBias)
            , clampBias(clampBias)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandSetLineWidth : public RHICommand
    {
        float width;

        RHICommandSetLineWidth(float width)
            : width(width)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandSetGraphicsPipeline : public RHICommand
    {
        RHIGraphicsPipelineRef graphicsPipeline;

        RHICommandSetGraphicsPipeline(RHIGraphicsPipelineRef graphicsPipeline)
            : graphicsPipeline(graphicsPipeline)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandSetComputePipeline : public RHICommand
    {
        RHIComputePipelineRef computePipeline;

        RHICommandSetComputePipeline(RHIComputePipelineRef computePipeline)
            : computePipeline(computePipeline)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    //struct RHICommandSetRayTracingPipeline : public RHICommand
    //{
    //   //  RHIRayTracingPipelineRef rayTracingPipeline;

    //    RHICommandSetRayTracingPipeline(RHIRayTracingPipelineRef rayTracingPipeline)
    //        : rayTracingPipeline(rayTracingPipeline)
    //    {
    //    }

    //    virtual void Execute(RHICommandContextRef context) override final;
    //};

    struct RHICommandPushConstants : public RHICommand
    {
        uint8_t data[256] = { 0 };  // 需要缓存push constant的数据，假定最大支持256字节
        uint16_t size;
        ShaderFrequency frequency;

        RHICommandPushConstants(void* data, uint16_t size, ShaderFrequency frequency)
            : size(size)
            , frequency(frequency)
        {
            assert(size <= 256);
            memcpy(&this->data[0], data, size);
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandBindDescriptorSet : public RHICommand
    {
        RHIDescriptorSetRef descriptor;
        uint32_t set;

        RHICommandBindDescriptorSet(RHIDescriptorSetRef descriptor, uint32_t set)
            : descriptor(descriptor)
            , set(set)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandBindVertexBuffer : public RHICommand
    {
        RHIBufferRef vertexBuffer;
        uint32_t streamIndex;
        uint32_t offset;

        RHICommandBindVertexBuffer(RHIBufferRef vertexBuffer, uint32_t streamIndex, uint32_t offset)
            : vertexBuffer(vertexBuffer)
            , streamIndex(streamIndex)
            , offset(offset)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandBindIndexBuffer : public RHICommand
    {
        RHIBufferRef indexBuffer;
        uint32_t offset;

        RHICommandBindIndexBuffer(RHIBufferRef indexBuffer, uint32_t offset)
            : indexBuffer(indexBuffer)
            , offset(offset)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandDispatch : public RHICommand
    {
        uint32_t groupCountX;
        uint32_t groupCountY;
        uint32_t groupCountZ; 

            RHICommandDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
            : groupCountX(groupCountX)
            , groupCountY(groupCountY)
            , groupCountZ(groupCountZ)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandDispatchIndirect : public RHICommand
    {
        RHIBufferRef argumentBuffer;
        uint32_t argumentOffset;

        RHICommandDispatchIndirect(RHIBufferRef argumentBuffer, uint32_t argumentOffset)
            : argumentBuffer(argumentBuffer)
            , argumentOffset(argumentOffset)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandTraceRays : public RHICommand
    {
        uint32_t groupCountX;
        uint32_t groupCountY;
        uint32_t groupCountZ;

        RHICommandTraceRays(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
            : groupCountX(groupCountX)
            , groupCountY(groupCountY)
            , groupCountZ(groupCountZ)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandDraw : public RHICommand
    {
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;

        RHICommandDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
            : vertexCount(vertexCount)
            , instanceCount(instanceCount)
            , firstVertex(firstVertex)
            , firstInstance(firstInstance)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandDrawIndexed : public RHICommand
    {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        uint32_t vertexOffset;
        uint32_t firstInstance;

        RHICommandDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
            : indexCount(indexCount)
            , instanceCount(instanceCount)
            , firstIndex(firstIndex)
            , vertexOffset(vertexOffset)
            , firstInstance(firstInstance)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandDrawIndirect : public RHICommand
    {
        RHIBufferRef argumentBuffer;
        uint32_t offset;
        uint32_t drawCount;

        RHICommandDrawIndirect(RHIBufferRef argumentBuffer, uint32_t offset, uint32_t drawCount)
            : argumentBuffer(argumentBuffer)
            , offset(offset)
            , drawCount(drawCount)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    struct RHICommandDrawIndexedIndirect : public RHICommand
    {
        RHIBufferRef argumentBuffer;
        uint32_t offset;
        uint32_t drawCount;

        RHICommandDrawIndexedIndirect(RHIBufferRef argumentBuffer, uint32_t offset, uint32_t drawCount)
            : argumentBuffer(argumentBuffer)
            , offset(offset)
            , drawCount(drawCount)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };
    struct RHICommandImGuiRenderDrawData : public RHICommand
    {
        RHICommandImGuiRenderDrawData() {}

        virtual void Execute(RHICommandContextRef context) override final;
    };
    struct RHICommandPushLabel : public RHICommand
    {
        std::string name;
        Color3 color;

        RHICommandPushLabel(const std::string& name, Color3 color)
            : name(name)
            , color(color)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };
    struct RHICommandPopLabel : public RHICommand
    {
        RHICommandPopLabel() {}

        virtual void Execute(RHICommandContextRef context) override final;
    };
    struct RHICommandSetRayTracingPipeline : public RHICommand
    {
        RHIRayTracingPipelineRef rayTracingPipeline;

        RHICommandSetRayTracingPipeline(RHIRayTracingPipelineRef rayTracingPipeline)
            : rayTracingPipeline(rayTracingPipeline)
        {
        }

        virtual void Execute(RHICommandContextRef context) override final;
    };

    /*RHICommandList是RHICommand的载体，方法通过RHICommandList调用后会根据配置选择是直接执行命令还是缓存命令*/
    class RHICommandList {
    public:
        RHICommandList(const CommandListInfo& info) : info(info) {}
        void BeginCommand();
        void EndCommand();
        void TextureBarrier(const RHITextureBarrier& barrier);
        void BeginRenderPass(RHIRenderPassRef renderPass);
        void CopyTexture(RHITextureRef src, TextureSubresourceLayers srcSubresource, RHITextureRef dst, TextureSubresourceLayers dstSubresource);

        void GenerateMips(RHITextureRef src);
        void EndRenderPass();
        void BufferBarrier(const RHIBufferBarrier& barrier);
        void Execute(RHIFenceRef fence = nullptr, RHISemaphoreRef waitSemaphore = nullptr, RHISemaphoreRef signalSemaphore = nullptr);
        void SetViewport(Offset2D min, Offset2D max);

        void SetScissor(Offset2D min, Offset2D max);

        void SetDepthBias(float constantBias, float slopeBias, float clampBias);
        void PushLabel(const std::string& name, Color3 color = { 1.0f, 1.0f, 1.0f });
        void PopLabel();
        void SetLineWidth(float width);

        void SetGraphicsPipeline(RHIGraphicsPipelineRef graphicsPipeline);
        void SetRayTracingPipeline(RHIRayTracingPipelineRef rayTracingPipeline);

        void SetComputePipeline(RHIComputePipelineRef computePipeline);

        // void SetRayTracingPipeline(RHIRayTracingPipelineRef rayTracingPipeline);

        void PushConstants(void* data, uint16_t size, ShaderFrequency frequency);

        void BindDescriptorSet(RHIDescriptorSetRef descriptor, uint32_t set = 0);

        void BindVertexBuffer(RHIBufferRef vertexBuffer, uint32_t streamIndex = 0, uint32_t offset = 0);

        void BindIndexBuffer(RHIBufferRef indexBuffer, uint32_t offset = 0);

        void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

        void DispatchIndirect(RHIBufferRef argumentBuffer, uint32_t argumentOffset = 0);

        void TraceRays(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);

        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t vertexOffset = 0, uint32_t firstInstance = 0);

        void DrawIndirect(RHIBufferRef argumentBuffer, uint32_t offset, uint32_t drawCount);

        void DrawIndexedIndirect(RHIBufferRef argumentBuffer, uint32_t offset, uint32_t drawCount);
        void ImGuiRenderDrawData();
        std::vector<RHIGPUTimeInfo> GetGPUTime();
    protected:
        inline void AddCommand(RHICommand* command) { commands.push_back(command); }


    private:
        std::vector<RHICommand*> commands;
        CommandListInfo info;
#if ENABLE_DEBUG_MODE
        int currentCommandIndex = 0;
#endif
    };

    struct RHICommandImmediateUploadImGuiFonts : public RHICommandImmediate
    {
        RHICommandImmediateUploadImGuiFonts() {}

        virtual void Execute(RHICommandContextImmediateRef context) override final;
    };
    struct RHICommandImmediateCopyBufferToTexture : public RHICommandImmediate
    {
        RHIBufferRef src;
        uint64_t srcOffset;
        RHITextureRef dst;
        TextureSubresourceLayers dstSubresource;

        RHICommandImmediateCopyBufferToTexture(RHIBufferRef src, uint64_t srcOffset, RHITextureRef dst, TextureSubresourceLayers dstSubresource)
            : src(src)
            , srcOffset(srcOffset)
            , dst(dst)
            , dstSubresource(dstSubresource)
        {
        }

        virtual void Execute(RHICommandContextImmediateRef context) override final;
    };

    class RHICommandListImmediate
    {
    public:
        RHICommandListImmediate(const CommandListImmediateInfo& info) : info(info) {}

        void Flush(); // 立即执行缓存的API命令
        void GenerateMips(RHITextureRef src);
        void TextureBarrier(const RHITextureBarrier& barrier);
        void CopyBufferToTexture(RHIBufferRef src, uint64_t srcOffset, RHITextureRef dst, TextureSubresourceLayers dstSubresource);
        void UploadImGuiFonts();


    protected:
        CommandListImmediateInfo info;

        inline void AddCommand(RHICommandImmediate* command) { commands.push_back(command); }
        std::vector<RHICommandImmediate*> commands;
    };

#define ADD_COMMAND(commandName, ...) do { \
    RHICommand* command = new RHICommand##commandName(__VA_ARGS__);  \
    AddCommand(command); \
} while (0)
#define ADD_COMMAND_IMMEDIATE(commandName, ...) do { \
    RHICommandImmediate* command = new RHICommandImmediate##commandName(__VA_ARGS__);  \
    AddCommand(command); \
} while (0)







}
