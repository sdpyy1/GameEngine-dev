#pragma once
#include "DependencyGraph.h"
#include <Hazel/Renderer/RHI/RHIBase.h>
namespace GameEngine
{
	// 所有RDG边均为 { 资源节点, pass节点 }   子类定义了纹理资源和Buffer资源的边
    enum RDGEdgeType
    {
        RDG_EDGE_TYPE_TEXTURE = 0,
        RDG_EDGE_TYPE_BUFFER,

        RDG_EDGE_TYPE_MAX_ENUM,    //
    };

    class RDGEdge : public DependencyGraph::Edge
    {
    public:
        RDGEdge(RDGEdgeType edgeType, RHIResourceState state = RESOURCE_STATE_UNDEFINED): state(state), edgeType(edgeType){}

        virtual bool IsOutput() { return false; }

        RDGEdgeType EdgeType() { return edgeType; }

        RHIResourceState state; // 在对应的pass处要求的状态（若作为pass输入，pass不应在内部改变状态）

    protected:
        RDGEdgeType edgeType;
    };
    typedef RDGEdge* RDGEdgeRef;

    class RDGTextureEdge : public RDGEdge
    {
    public:
        RDGTextureEdge()
            : RDGEdge(RDG_EDGE_TYPE_TEXTURE)
        {
        }

        TextureSubresourceRange subresource = {};
        TextureSubresourceLayers subresourceLayer = {};  // 单层mip，多层layer

        // 下面这些每种标记都对应一种用法
        bool asColor = false;   // 颜色缓冲标记
        bool asDepthStencil = false; // 深度缓冲标记
        bool asShaderRead = false;
        bool asShaderReadWrite = false;
        bool asOutputRead = false;
        bool asOutputReadWrite = false;
        bool asPresent = false;
        bool asTransferSrc = false;
        bool asTransferDst = false;

        virtual bool IsOutput() override { return asOutputRead || asOutputReadWrite; }

        uint32_t set = 0; 
        uint32_t binding = 0;
        uint32_t index = 0;
        ResourceType type = RESOURCE_TYPE_TEXTURE;
        TextureViewType viewType = VIEW_TYPE_2D;

        AttachmentLoadOp 	loadOp = ATTACHMENT_LOAD_OP_DONT_CARE; 
        AttachmentStoreOp	storeOp = ATTACHMENT_STORE_OP_DONT_CARE;

        Color4				clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
        float				clearDepth = 1.0f;
        uint32_t			clearStencil = 0;
    };
    typedef RDGTextureEdge* RDGTextureEdgeRef;

    class RDGBufferEdge : public RDGEdge
    {
    public:
        RDGBufferEdge()
            : RDGEdge(RDG_EDGE_TYPE_BUFFER)
        {
        }

        uint32_t offset = 0;
        uint32_t size = 0;
        bool asShaderRead = false;   // Shader会读取
        bool asShaderReadWrite = false;
        bool asOutputRead = false;
        bool asOutputReadWrite = false;
        bool asOutputIndirectDraw = false;

        virtual bool IsOutput() override { return asOutputRead || asOutputReadWrite || asOutputIndirectDraw; }

        uint32_t set = 0;       // 描述符使用
        uint32_t binding = 0;
        uint32_t index = 0;
        ResourceType type = RESOURCE_TYPE_UNIFORM_BUFFER;
    };
    typedef RDGBufferEdge* RDGBufferEdgeRef;
}
