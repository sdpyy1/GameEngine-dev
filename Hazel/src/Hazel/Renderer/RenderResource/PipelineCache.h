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
					return  MurmurHash64A(&a.info, sizeof(RHIGraphicsPipelineInfo), 0);
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
