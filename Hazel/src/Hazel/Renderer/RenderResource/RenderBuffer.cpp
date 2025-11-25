#include "hzpch.h"
#include "RenderBuffer.h"
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
namespace GameEngine {

	namespace V2 { 
        VertexBuffer::VertexBuffer()
        {
            vertexID = RENDER_RESOURCEMANAGER->AllocateVertexID();
        }

        void VertexBuffer::SetPosition(const std::vector<glm::vec3>& position)
        {
            SetBufferData(
                (void*)position.data(),
                position.size() * sizeof(glm::vec3),
                positionBuffer,
                vertexInfo.positionID,
                BINDLESS_SLOT_POSITION);

            vertexNum = position.size();    // 存一下当前的顶点数目，以position为基准
        }
        void VertexBuffer::SetNormal(const std::vector<glm::vec3>& normal)
        {
            SetBufferData(
                (void*)normal.data(),
                normal.size() * sizeof(glm::vec3),
                normalBuffer,
                vertexInfo.normalID,
                BINDLESS_SLOT_NORMAL);
        }

        void VertexBuffer::SetTangent(const std::vector<glm::vec4>& tangent)
        {
            SetBufferData(
                (void*)tangent.data(),
                tangent.size() * sizeof(glm::vec4),
                tangentBuffer,
                vertexInfo.tangentID,
                BINDLESS_SLOT_TANGENT);
        }

        void VertexBuffer::SetTexCoord(const std::vector<glm::vec2>& texCoord)
        {
            SetBufferData(
                (void*)texCoord.data(),
                texCoord.size() * sizeof(glm::vec2),
                texCoordBuffer,
                vertexInfo.texCoordID,
                BINDLESS_SLOT_TEXCOORD);
        }

        void VertexBuffer::SetColor(const std::vector<glm::vec3>& color)
        {
            SetBufferData(
                (void*)color.data(),
                color.size() * sizeof(glm::vec3),
                colorBuffer,
                vertexInfo.colorID,
                BINDLESS_SLOT_COLOR);
        }

        void VertexBuffer::SetBoneIndex(const std::vector<glm::ivec4>& boneIndex)
        {
            SetBufferData(
                (void*)boneIndex.data(),
                boneIndex.size() * sizeof(glm::ivec4),
                boneIndexBuffer,
                vertexInfo.boneIndexID,
                BINDLESS_SLOT_BONE_INDEX);
        }

        void VertexBuffer::SetBoneWeight(const std::vector<glm::vec4>& boneWeight)
        {
            SetBufferData(
                (void*)boneWeight.data(),
                boneWeight.size() * sizeof(glm::vec4),
                boneWeightBuffer,
                vertexInfo.boneWeightID,
                BINDLESS_SLOT_BONE_WEIGHT);
        }

        void VertexBuffer::SetBufferData(void* data, uint32_t size, RHIBufferRef& buffer, uint32_t& id, uint32_t slot)
        {
            if (size == 0) return;
            if (!buffer || buffer->GetInfo().size < size)  // 创建buffer
            {
                RHIBufferInfo rHIBufferInfo;
                rHIBufferInfo.type = RESOURCE_TYPE_RW_BUFFER | RESOURCE_TYPE_VERTEX_BUFFER;
                rHIBufferInfo.size = size;
                rHIBufferInfo.memoryUsage = MEMORY_USAGE_CPU_TO_GPU;
                rHIBufferInfo.creationFlag = BUFFER_CREATION_PERSISTENT_MAP;
                buffer = APP_DYNAMICRHI->CreateBuffer(rHIBufferInfo);

                if (id != 0) RENDER_RESOURCEMANAGER->ReleaseBindlessID(id, (BindlessSlot)slot);
                BindlessResourceInfo bindlessResourceInfo;
                bindlessResourceInfo.buffer = buffer;
                bindlessResourceInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
                bindlessResourceInfo.bufferOffset = 0;
                bindlessResourceInfo.bufferRange = size;

                id = RENDER_RESOURCEMANAGER->AllocateBindlessID(bindlessResourceInfo, (BindlessSlot)slot);
            }

            memcpy(buffer->Map(), data, size);

            RENDER_RESOURCEMANAGER->SetVertexInfo(vertexInfo, vertexID);
        }

		void IndexBuffer::SetIndex(const std::vector<uint32_t>& index)
		{
            indexNum = index.size();
            uint32_t size = index.size() * sizeof(uint32_t);

            if (size == 0) return;
            if (!buffer || buffer->GetInfo().size < size)  // 创建buffer
            {
                RHIBufferInfo rHIBufferInfo;
                rHIBufferInfo.type = RESOURCE_TYPE_RW_BUFFER | RESOURCE_TYPE_INDEX_BUFFER;
                rHIBufferInfo.size = size;
                rHIBufferInfo.memoryUsage = MEMORY_USAGE_CPU_TO_GPU;
                rHIBufferInfo.creationFlag = BUFFER_CREATION_PERSISTENT_MAP;
    
                buffer = APP_DYNAMICRHI->CreateBuffer(rHIBufferInfo);


                if (indexID != 0) RENDER_RESOURCEMANAGER->ReleaseBindlessID(indexID, BINDLESS_SLOT_INDEX);
                BindlessResourceInfo bindlessResourceInfo;
                bindlessResourceInfo.buffer = buffer;
                bindlessResourceInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
                bindlessResourceInfo.bufferOffset = 0;
                bindlessResourceInfo.bufferRange = size;
                indexID = RENDER_RESOURCEMANAGER->AllocateBindlessID(bindlessResourceInfo,BINDLESS_SLOT_INDEX);
            }

            memcpy(buffer->Map(), index.data(), size);
		}

	}

}