#pragma once
#include "DependencyGraph.h"
#include "RDGHandle.h"
#include "RDGNode.h"
#include "Hazel/Renderer/RHI/RHICommandList.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
// #define RDG_DEBUG
/*
    RDGNode分为PassNode(RenderPass, ComputePass, RayTracingPass, CopyPass, PresentPass)和ResourceNode(TextureNode, BufferNode)
    RDGEdge用于存储 资源->Pass的View，用于在Pass时创建对应的view
*/
namespace GameEngine {

    // name->node
    class RDGBlackBoard
    {
    public:
        RDGPassNodeRef Pass(std::string name);
        RDGBufferNodeRef Buffer(std::string name);
        RDGTextureNodeRef Texture(std::string name);
#ifdef RDG_DEBUG
        std::string PassName(RDGPassNodeRef pass);
        std::string BufferName(RDGBufferNodeRef buffer);
        std::string TextureName(RDGTextureNodeRef texture);
#endif
        void AddPass(RDGPassNodeRef pass);
        void AddBuffer(RDGBufferNodeRef buffer);
        void AddTexture(RDGTextureNodeRef texture);

        void Clear() { passes.clear(); buffers.clear(); textures.clear(); }

    private:
        std::unordered_map<std::string, RDGPassNodeRef> passes;
        std::unordered_map<std::string, RDGBufferNodeRef> buffers;
        std::unordered_map<std::string, RDGTextureNodeRef> textures;
#ifdef RDG_DEBUG
        std::unordered_map<RDGPassNodeRef,std::string> passeNames;
        std::unordered_map<RDGTextureNodeRef,std::string> textureNames;
        std::unordered_map<RDGBufferNodeRef,std::string> bufferNames;
#endif

    };

    class RDGBuilder
    {
    public:
        RDGBuilder() = default;
        RDGBuilder(RHICommandListRef command): command(command){}  // 给一个commandList

        ~RDGBuilder() {};

        RDGTextureBuilder CreateTexture(std::string name);
        RDGBufferBuilder CreateBuffer(std::string name);
        RDGRenderPassBuilder CreateRenderPass(std::string name);    // 假定所有pass的添加顺序就是执行顺序，方便处理排序和依赖关系等
        RDGComputePassBuilder CreateComputePass(std::string name);
        RDGRayTracingPassBuilder CreateRayTracingPass(std::string name);
        RDGPresentPassBuilder CreatePresentPass(std::string name);
        RDGCopyPassBuilder CreateCopyPass(std::string name);

        RDGTextureHandle GetTexture(std::string name);
        RDGBufferHandle GetBuffer(std::string name);
        RDGRenderPassHandle GetRenderPass(std::string name) { return GetPass<RDGRenderPassNodeRef, RDGRenderPassHandle>(name); }
        RDGComputePassHandle GetComputePass(std::string name) { return GetPass<RDGComputePassNodeRef, RDGComputePassHandle>(name); }
        RDGRayTracingPassHandle GetRayTracingPass(std::string name) { return GetPass<RDGRayTracingPassNodeRef, RDGRayTracingPassHandle>(name); }
        RDGPresentPassHandle GetPresentPass(std::string name) { return GetPass<RDGPresentPassNodeRef, RDGPresentPassHandle>(name); }
        RDGCopyPassHandle GetCopyPass(std::string name) { return GetPass<RDGCopyPassNodeRef, RDGCopyPassHandle>(name); }

        RHITextureRef GetRHITexture(std::string name) { return blackBoard.Texture(name)->GetRHITexture(); }
        DependencyGraphRef GetGraph() { return graph; }

        void Execute();

    private:
        void CreateInputBarriers(RDGPassNodeRef pass);
        void CreateOutputBarriers(RDGPassNodeRef pass);
        void PrepareDescriptorSet(RDGPassNodeRef pass);
        void PrepareRenderTarget(RDGRenderPassNodeRef pass, RHIRenderPassInfo& renderPassInfo);
        void ReleaseResource(RDGPassNodeRef pass);

        void ExecutePass(RDGRenderPassNodeRef pass);
        void ExecutePass(RDGComputePassNodeRef pass);
        void ExecutePass(RDGRayTracingPassNodeRef pass);
        void ExecutePass(RDGPresentPassNodeRef pass);
        void ExecutePass(RDGCopyPassNodeRef pass);

        template<typename Type, typename Handle>
        Handle GetPass(std::string name)
        {
            auto node = blackBoard.Pass(name);
            if (node == nullptr)
            {
                LOG_TRACE("Unable to find RDG resource, please check name!");
                return Handle(UINT32_MAX);
            }
            return dynamic_cast<Type>(node)->GetHandle();
        }
        // Resolve 创建真正的资源
        RHITextureRef Resolve(RDGTextureNodeRef textureNode);
        RHIBufferRef Resolve(RDGBufferNodeRef bufferNode);
        void Release(RDGTextureNodeRef textureNode, RHIResourceState state);
        void Release(RDGBufferNodeRef bufferNode, RHIResourceState state);

        RHIResourceState PreviousState(RDGTextureNodeRef textureNode, RDGPassNodeRef passNode, TextureSubresourceRange subresource = {}, bool output = false);    // 获取当前pass（在执行顺序上）的资源的前序状态
        RHIResourceState PreviousState(RDGBufferNodeRef bufferNode, RDGPassNodeRef passNode, uint32_t offset = 0, uint32_t size = 0, bool output = false);      // output用于标记是相对于输入还是输出资源
        bool IsLastUsedPass(RDGTextureNodeRef textureNode, RDGPassNodeRef passNode, bool output = false);
        bool IsLastUsedPass(RDGBufferNodeRef bufferNode, RDGPassNodeRef passNode, bool output = false);

        std::vector<RDGPassNodeRef> passes; // 创建的全部pass，按照创建顺序执行

        DependencyGraphRef graph = std::make_shared<DependencyGraph>();
        RDGBlackBoard blackBoard;

        RHICommandListRef command;
    };

    class RDGTextureBuilder
    {
    public:
        RDGTextureBuilder(RDGBuilder* builder, RDGTextureNodeRef texture)
            : builder(builder)
            , texture(texture) {
        };

        RDGTextureBuilder& Import(RHITextureRef texture, RHIResourceState initState);
        RDGTextureBuilder& Exetent(Extent3D extent);
        RDGTextureBuilder& Format(RHIFormat format);
        RDGTextureBuilder& MemoryUsage(MemoryUsage memoryUsage);
        RDGTextureBuilder& AllowReadWrite();
        RDGTextureBuilder& AllowRenderTarget();
        RDGTextureBuilder& AllowDepthStencil();
        RDGTextureBuilder& MipLevels(uint32_t mipLevels);
        RDGTextureBuilder& ArrayLayers(uint32_t arrayLayers);
        RDGTextureBuilder& CubeMap();

        RDGTextureHandle Finish() { return texture->GetHandle(); }

    private:
        RDGBuilder* builder;
        RDGTextureNodeRef texture;
    };

    class RDGBufferBuilder
    {
    public:
        RDGBufferBuilder(RDGBuilder* builder, RDGBufferNodeRef buffer)
            : builder(builder)
            , buffer(buffer) {
        };

        RDGBufferBuilder& Import(RHIBufferRef buffer, RHIResourceState initState);
        RDGBufferBuilder& Size(uint32_t size);
        RDGBufferBuilder& MemoryUsage(MemoryUsage memoryUsage);
        RDGBufferBuilder& AllowVertexBuffer();
        RDGBufferBuilder& AllowIndexBuffer();
        RDGBufferBuilder& AllowReadWrite();
        RDGBufferBuilder& AllowRead();

        RDGBufferHandle Finish() { return buffer->GetHandle(); }

    private:
        RDGBuilder* builder;
        RDGBufferNodeRef buffer;
    };

    class RDGRenderPassBuilder
    {
    public:
        RDGRenderPassBuilder(RDGBuilder* builder, RDGRenderPassNodeRef pass)
            : builder(builder)
            , pass(pass)
            , graph(builder->GetGraph()) {
        };

        RDGRenderPassBuilder& PassIndex(uint32_t x = 0, uint32_t y = 0, uint32_t z = 0);        // 给一个index设置函数方便给Execute传参
        RDGRenderPassBuilder& RootSignature(RHIRootSignatureRef rootSignature);                 // 若提供根签名未提供描述符，使用池化创建
        RDGRenderPassBuilder& DescriptorSet(uint32_t set, RHIDescriptorSetRef descriptorSet);   // 若提供了描述符，直接用相应的描述符
        
        // 创建边  默认View都是2D，如果需要cubeView，需要手动指定
        RDGRenderPassBuilder& Read(uint32_t set, uint32_t binding, uint32_t index, RDGBufferHandle buffer, uint32_t offset = 0, uint32_t size = 0);
        RDGRenderPassBuilder& Read(uint32_t set, uint32_t binding, uint32_t index, RDGTextureHandle texture, TextureViewType viewType = VIEW_TYPE_2D, TextureSubresourceRange subresource = {});
        RDGRenderPassBuilder& ReadWrite(uint32_t set, uint32_t binding, uint32_t index, RDGBufferHandle buffer, uint32_t offset = 0, uint32_t size = 0);   // 好像和read也没什么区别？
        RDGRenderPassBuilder& ReadWrite(uint32_t set, uint32_t binding, uint32_t index, RDGTextureHandle texture, TextureViewType viewType = VIEW_TYPE_2D, TextureSubresourceRange subresource = {});
        RDGRenderPassBuilder& LabelColor(Color3 color);
        RDGRenderPassBuilder& Color(uint32_t binding, RDGTextureHandle texture,
            AttachmentLoadOp load = ATTACHMENT_LOAD_OP_DONT_CARE,
            AttachmentStoreOp store = ATTACHMENT_STORE_OP_DONT_CARE,
            Color4 clearColor = { 0.0f, 0.0f, 0.0f, 0.0f },
            TextureSubresourceRange subresource = {});
        RDGRenderPassBuilder& DepthStencil(RDGTextureHandle texture,
            AttachmentLoadOp load = ATTACHMENT_LOAD_OP_DONT_CARE,
            AttachmentStoreOp store = ATTACHMENT_STORE_OP_DONT_CARE,
            float clearDepth = 1.0f,
            uint32_t clearStencil = 0,
            TextureSubresourceRange subresource = {});

        // 标识后续还会使用的资源，会用PassNode->ResourceNode
        RDGRenderPassBuilder& OutputRead(RDGBufferHandle buffer, uint32_t offset = 0, uint32_t size = 0);             // 在执行完Pass后作为输出，自动屏障，可能还会在其他地方使用
        RDGRenderPassBuilder& OutputRead(RDGTextureHandle texture, TextureSubresourceRange subresource = {});
        RDGRenderPassBuilder& OutputReadWrite(RDGBufferHandle buffer, uint32_t offset = 0, uint32_t size = 0);
        RDGRenderPassBuilder& OutputReadWrite(RDGTextureHandle texture, TextureSubresourceRange subresource = {});
        RDGRenderPassBuilder& Execute(const RDGPassExecuteFunc& execute);

        RDGRenderPassHandle Finish() { return pass->GetHandle(); }

    private:
        RDGBuilder* builder;
        RDGRenderPassNodeRef pass;

        DependencyGraphRef graph;
    };

    class RDGComputePassBuilder
    {
    public:
        RDGComputePassBuilder(RDGBuilder* builder, RDGComputePassNodeRef pass)
            : builder(builder)
            , pass(pass)
            , graph(builder->GetGraph()) {
        };

        RDGComputePassBuilder& PassIndex(uint32_t x = 0, uint32_t y = 0, uint32_t z = 0);        // 给一个index设置函数方便给Execute传参
        RDGComputePassBuilder& RootSignature(RHIRootSignatureRef rootSignature);                 // 若提供根签名未提供描述符，使用池化创建
        RDGComputePassBuilder& DescriptorSet(uint32_t set, RHIDescriptorSetRef descriptorSet);   // 若提供了描述符，直接用相应的描述符
        RDGComputePassBuilder& Read(uint32_t set, uint32_t binding, uint32_t index, RDGBufferHandle buffer, uint32_t offset = 0, uint32_t size = 0);
        RDGComputePassBuilder& Read(uint32_t set, uint32_t binding, uint32_t index, RDGTextureHandle texture, TextureViewType viewType = VIEW_TYPE_2D, TextureSubresourceRange subresource = {});
        RDGComputePassBuilder& ReadWrite(uint32_t set, uint32_t binding, uint32_t index, RDGBufferHandle buffer, uint32_t offset = 0, uint32_t size = 0);   // 好像和read也没什么区别？
        RDGComputePassBuilder& ReadWrite(uint32_t set, uint32_t binding, uint32_t index, RDGTextureHandle texture, TextureViewType viewType = VIEW_TYPE_2D, TextureSubresourceRange subresource = {});
        RDGComputePassBuilder& OutputRead(RDGBufferHandle buffer, uint32_t offset = 0, uint32_t size = 0);             // 在执行完Pass后作为输出，自动屏障，可能还会在其他地方使用
        RDGComputePassBuilder& OutputRead(RDGTextureHandle texture, TextureSubresourceRange subresource = {});
        RDGComputePassBuilder& OutputReadWrite(RDGBufferHandle buffer, uint32_t offset = 0, uint32_t size = 0);
        RDGComputePassBuilder& OutputReadWrite(RDGTextureHandle texture, TextureSubresourceRange subresource = {});
        RDGComputePassBuilder& OutputIndirectDraw(RDGBufferHandle buffer, uint32_t offset = 0, uint32_t size = 0);


        RDGComputePassBuilder& Execute(const RDGPassExecuteFunc& execute);

        RDGComputePassHandle Finish() { return pass->GetHandle(); }

    private:
        RDGBuilder* builder;
        RDGComputePassNodeRef pass;

        DependencyGraphRef graph;
    };

    class RDGRayTracingPassBuilder
    {
    public:
        RDGRayTracingPassBuilder(RDGBuilder* builder, RDGRayTracingPassNodeRef pass)
            : builder(builder)
            , pass(pass)
            , graph(builder->GetGraph()) {
        };

        RDGRayTracingPassBuilder& PassIndex(uint32_t x = 0, uint32_t y = 0, uint32_t z = 0);        // 给一个index设置函数方便给Execute传参
        RDGRayTracingPassBuilder& RootSignature(RHIRootSignatureRef rootSignature);                 // 若提供根签名未提供描述符，使用池化创建
        RDGRayTracingPassBuilder& DescriptorSet(uint32_t set, RHIDescriptorSetRef descriptorSet);   // 若提供了描述符，直接用相应的描述符
        RDGRayTracingPassBuilder& Read(uint32_t set, uint32_t binding, uint32_t index, RDGBufferHandle buffer, uint32_t offset = 0, uint32_t size = 0);
        RDGRayTracingPassBuilder& Read(uint32_t set, uint32_t binding, uint32_t index, RDGTextureHandle texture, TextureViewType viewType = VIEW_TYPE_2D, TextureSubresourceRange subresource = {});
        RDGRayTracingPassBuilder& ReadWrite(uint32_t set, uint32_t binding, uint32_t index, RDGBufferHandle buffer, uint32_t offset = 0, uint32_t size = 0);   // 好像和read也没什么区别？
        RDGRayTracingPassBuilder& ReadWrite(uint32_t set, uint32_t binding, uint32_t index, RDGTextureHandle texture, TextureViewType viewType = VIEW_TYPE_2D, TextureSubresourceRange subresource = {});
        RDGRayTracingPassBuilder& OutputRead(RDGBufferHandle buffer, uint32_t offset = 0, uint32_t size = 0);             // 在执行完Pass后作为输出，自动屏障，可能还会在其他地方使用
        RDGRayTracingPassBuilder& OutputRead(RDGTextureHandle texture, TextureSubresourceRange subresource = {});
        RDGRayTracingPassBuilder& OutputReadWrite(RDGBufferHandle buffer, uint32_t offset = 0, uint32_t size = 0);
        RDGRayTracingPassBuilder& OutputReadWrite(RDGTextureHandle texture, TextureSubresourceRange subresource = {});


        RDGRayTracingPassBuilder& Execute(const RDGPassExecuteFunc& execute);

        RDGRayTracingPassHandle Finish() { return pass->GetHandle(); }

    private:
        RDGBuilder* builder;
        RDGRayTracingPassNodeRef pass;

        DependencyGraphRef graph;
    };

    class RDGPresentPassBuilder
    {
    public:
        RDGPresentPassBuilder(RDGBuilder* builder, RDGPresentPassNodeRef pass)
            : builder(builder)
            , pass(pass)
            , graph(builder->GetGraph()) {
        };

        RDGPresentPassHandle Finish() { return pass->GetHandle(); }

        RDGPresentPassBuilder& Texture(RDGTextureHandle texture, TextureSubresourceLayers subresource = {});
        RDGPresentPassBuilder& PresentTexture(RDGTextureHandle texture);

    private:
        RDGBuilder* builder;
        RDGPresentPassNodeRef pass;

        DependencyGraphRef graph;
    };

    class RDGCopyPassBuilder
    {
    public:
        RDGCopyPassBuilder(RDGBuilder* builder, RDGCopyPassNodeRef pass)
            : builder(builder)
            , pass(pass)
            , graph(builder->GetGraph()) {
        };

        RDGCopyPassHandle Finish() { return pass->GetHandle(); }

        RDGCopyPassBuilder& From(RDGTextureHandle texture, TextureSubresourceLayers subresource = {});
        RDGCopyPassBuilder& To(RDGTextureHandle texture, TextureSubresourceLayers subresource = {});
        RDGCopyPassBuilder& GenerateMips();
        RDGCopyPassBuilder& OutputRead(RDGTextureHandle texture, TextureSubresourceLayers subresource = {});
        RDGCopyPassBuilder& OutputReadWrite(RDGTextureHandle texture, TextureSubresourceLayers subresource = {});

    private:
        RDGBuilder* builder;
        RDGCopyPassNodeRef pass;

        DependencyGraphRef graph;
    };




}

