#pragma once
#include "Hazel/Renderer/RHI/RHIBase.h"
#include "Hazel/Utils/HashUtils.h"
namespace GameEngine
{
	class GraphicsPipelineCache
	{
	public:
		struct CachedPipeline
		{
			RHIGraphicsPipelineRef pipeline;
		};
		struct Key
		{
			Key(const RHIGraphicsPipelineInfo& info) : info(info) {}
			RHIGraphicsPipelineInfo info;
			friend bool operator== (const Key& a, const Key& b)
			{
				return a.info == b.info;
			}
			struct Hash {
				size_t operator()(const Key& a) const {
					size_t h = 0;

					// 把指针值 reinterpret_cast 为 uint64_t，然后 hash
					uint64_t vptr = reinterpret_cast<uint64_t>(a.info.vertexShader.get());
					h ^= MurmurHash64A(&vptr, sizeof(uint64_t), 0);

					uint64_t gptr = reinterpret_cast<uint64_t>(a.info.geometryShader.get());
					h ^= MurmurHash64A(&gptr, sizeof(uint64_t), 0);

					uint64_t fptr = reinterpret_cast<uint64_t>(a.info.fragmentShader.get());
					h ^= MurmurHash64A(&fptr, sizeof(uint64_t), 0);

					uint64_t rptr = reinterpret_cast<uint64_t>(a.info.rootSignature.get());
					h ^= MurmurHash64A(&rptr, sizeof(uint64_t), 0);

					h ^= MurmurHash64A(&a.info.primitiveType, sizeof(PrimitiveType), 0);
					h ^= MurmurHash64A(&a.info.rasterizerState, sizeof(RHIRasterizerStateInfo), 0);
					h ^= MurmurHash64A(&a.info.blendState, sizeof(RHIBlendStateInfo), 0);
					h ^= MurmurHash64A(&a.info.depthStencilState, sizeof(RHIDepthStencilStateInfo), 0);
					// h ^= MurmurHash64A(&a.info.vertexInputState, sizeof(VertexInputStateInfo), 0);   这行没有设置初始值，会导致Hash不一致，目前缓存的pipeline都是不需要vertexInputState的

					return h;
				}
			};
		};


		CachedPipeline Allocate(const RHIGraphicsPipelineInfo& info);

		inline uint32_t CachedSize() { return cachedPipelines.size(); }
		void Clear() { cachedPipelines.clear(); }

		static std::shared_ptr<GraphicsPipelineCache> Get()
		{
			static std::shared_ptr<GraphicsPipelineCache> pool;
			if (pool == nullptr) pool = std::make_shared<GraphicsPipelineCache>();
			return pool;
		}

	private:
		std::unordered_map<Key, CachedPipeline, Key::Hash> cachedPipelines;

		bool IsValid(RHIGraphicsPipelineInfo info);

	};
}
