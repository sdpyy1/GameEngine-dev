#pragma once
#include "Hazel/Utils/Serializable.h"
namespace GameEngine {
	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID& other);

		operator uint64_t () { return m_UUID; }
		operator const uint64_t() const { return m_UUID; }
	private:
		uint64_t m_UUID;
		BeginSerailize
			SerailizeEntry(m_UUID)
		EndSerailize
	};
}

namespace std {
	// 定义UUID的哈希方法，直接用本身进行Hash
	template <>
	struct hash<GameEngine::UUID>
	{
		std::size_t operator()(const GameEngine::UUID& uuid) const
		{
			return uuid;
		}
	};
}
