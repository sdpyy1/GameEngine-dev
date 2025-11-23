#pragma once
#include "Hazel/Renderer/RHI/RHI.h"
#include "Hazel/Core/Application.h"
namespace GameEngine
{
    class VertexBuffer
    {
    public:
        VertexBuffer();
        ~VertexBuffer();

        void SetPosition(const std::vector<Vec3>& position);
        void SetNormal(const std::vector<Vec3>& normal);
        void SetTangent(const std::vector<Vec4>& tangent);
        void SetTexCoord(const std::vector<Vec2>& texCoord);
        void SetColor(const std::vector<Vec3>& color);
        void SetBoneIndex(const std::vector<IVec4>& boneIndex);
        void SetBoneWeight(const std::vector<Vec4>& boneWeight);

        RHIBufferRef positionBuffer;
        RHIBufferRef normalBuffer;
        RHIBufferRef tangentBuffer;
        RHIBufferRef texCoordBuffer;
        RHIBufferRef colorBuffer;
        RHIBufferRef boneIndexBuffer;
        RHIBufferRef boneWeightBuffer;

        uint32_t vertexID = 0;
        VertexInfo vertexInfo = {
            .positionID = 0,
            .normalID = 0,
            .texCoordID = 0,
            .colorID = 0,
            .boneIndexID = 0,
            .boneWeightID = 0
        };

        inline uint32_t VertexNum() { return vertexNum; }

    private:
        void SetBufferData(void* data, uint32_t size, RHIBufferRef& buffer, uint32_t& id, uint32_t slot);

        // RHIBufferRef stagingBuffer;

        uint32_t vertexNum = 0;
    };
    typedef std::shared_ptr<VertexBuffer> VertexBufferRef;

    class IndexBuffer
    {
    public:
        IndexBuffer() = default;
        ~IndexBuffer();

        void SetIndex(const std::vector<uint32_t>& index);

        RHIBufferRef buffer;

        uint32_t indexID = 0;

        inline uint32_t IndexNum() { return indexNum; }
        inline uint32_t TriangleNum() { return indexNum / 3; }

    private:
        uint32_t indexNum = 0;
    };
    typedef std::shared_ptr<IndexBuffer> IndexBufferRef;



	template<typename Type>   // 直接指明Buffer要存储的数据类型
	class RenderBuffer
	{

	public:
		RenderBuffer(ResourceType type = RESOURCE_TYPE_RW_BUFFER | RESOURCE_TYPE_UNIFORM_BUFFER, MemoryUsage usage = MEMORY_USAGE_CPU_TO_GPU)
		{
			m_Size = sizeof(Type);
			RHIBufferInfo info;
			info.size = sizeof(Type);
            info.memoryUsage = usage;
            info.type = type;
            info.creationFlag = BUFFER_CREATION_PERSISTENT_MAP;  // 加速map操作
			buffer = APP_DYNAMICRHI->CreateBuffer(info);
		}
		RHIBufferRef GetRHIBuffer() { return buffer; }
		void SetData(const Type& data)
		{
			memcpy(buffer->Map(), &data, sizeof(Type));
		}

		void SetData(const void* data, uint32_t size, uint32_t offset = 0)
		{
			memcpy((uint8_t*)buffer->Map() + offset, data, size);
		}

		void GetData(Type* data)
		{
			memcpy(data, buffer->Map(), sizeof(Type));
		}

		void GetData(void* data, uint32_t size, uint32_t offset = 0)
		{
			memcpy(data, (uint8_t*)buffer->Map() + offset, size);
		}
		uint32_t GetSize() { return m_Size; }
	private:
		RHIBufferRef buffer;
		uint32_t m_Size;
	};







}

