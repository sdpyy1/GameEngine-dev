#pragma once
#include "Hazel/Renderer/RHI/RHI.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Utils/IndexAllocator.h"
#include "RenderStruct.h"
#include "Hazel/Renderer/RenderSystem/RenderManager.h"
namespace GameEngine
{
	template<typename Type>   // 直接指明Buffer要存储的数据类型
	class RenderBuffer
	{
	public:
		RenderBuffer(ResourceType type = RESOURCE_TYPE_RW_BUFFER | RESOURCE_TYPE_UNIFORM_BUFFER, MemoryUsage usage = MEMORY_USAGE_CPU_TO_GPU) {
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

	// 用一个Buffer来管理数组，通过偏移来访问
	template<typename Type, size_t arraySize>
	class ArrayBuffer
	{
	public:
		ArrayBuffer(): idAlloctor(arraySize)
		{
			RHIBufferInfo info;
            info.type = RESOURCE_TYPE_RW_BUFFER | RESOURCE_TYPE_UNIFORM_BUFFER | RESOURCE_TYPE_INDIRECT_BUFFER;
            info.memoryUsage = MEMORY_USAGE_CPU_TO_GPU;
			info.creationFlag = BUFFER_CREATION_PERSISTENT_MAP;
            info.size = arraySize * sizeof(Type);
			buffer = APP_DYNAMICRHI->CreateBuffer(info);
		}

		void SetData(const Type& data, uint32_t index)
		{
			memcpy((Type*)buffer->Map() + index, &data, sizeof(Type));
		}

		void SetData(const std::vector<Type>& data, uint32_t index = 0)
		{
			memcpy((Type*)buffer->Map() + index, data.data(), data.size() * sizeof(Type));
		}

		uint32_t Allocate() { return idAlloctor.Allocate(); }
		IndexRange Allocate(uint32_t size) { return idAlloctor.Allocate(size); }
		void Release(uint32_t index) { idAlloctor.Release(index); }
		void Release(IndexRange range) { idAlloctor.Release(range); }
		RHIBufferRef GetRHIBuffer() { return buffer; }
		RHIBufferRef buffer;

	private:
		IndexAllocator idAlloctor;
	};
	template<typename Type, size_t arraySize>
	using ArrayBufferRef = std::shared_ptr<ArrayBuffer<Type, arraySize>>;

	// 一个Mesh解析出来的顶点数据被分别存储在多个Bindless位置，这里存一份各种信息的Bindless的ID
	class VertexBuffer {
	public:
		VertexBuffer();
		~VertexBuffer();
		void SetPosition(const std::vector<glm::vec3>& position);
		void SetNormal(const std::vector<glm::vec3>& normal);
		void SetTangent(const std::vector<glm::vec4>& tangent);
		void SetTexCoord(const std::vector<glm::vec2>& texCoord);
		void SetColor(const std::vector<glm::vec3>& color);
		void SetBoneIndex(const std::vector<glm::ivec4>& boneIndex);
		void SetBoneWeight(const std::vector<glm::vec4>& boneWeight);
		void SetBoundingBox(const BoundingBox& box);

		RHIBufferRef positionBuffer;
		RHIBufferRef normalBuffer;
		RHIBufferRef tangentBuffer;
		RHIBufferRef texCoordBuffer;
		RHIBufferRef colorBuffer;
		RHIBufferRef boneIndexBuffer;
		RHIBufferRef boneWeightBuffer;
		MeshInfo vertexInfo;
		uint32_t vertexID = 0;
		inline uint32_t VertexNum() { return vertexNum; }

	private:
		void SetBufferData(void* data, uint32_t size, RHIBufferRef& buffer, uint32_t& id, uint32_t slot);
		uint32_t vertexNum = 0;

	};
	class IndexBuffer
	{
	public:
		IndexBuffer() {};
		~IndexBuffer();

		void SetIndex(const std::vector<uint32_t>& index);

		RHIBufferRef buffer;

		uint32_t indexID = 0;

		inline uint32_t IndexNum() { return indexNum; }
		inline uint32_t TriangleNum() { return indexNum / 3; }

	private:
		uint32_t indexNum = 0;
	};
	using VertexBufferRef = std::shared_ptr<VertexBuffer>;
	using IndexBufferRef = std::shared_ptr<IndexBuffer>;

}

