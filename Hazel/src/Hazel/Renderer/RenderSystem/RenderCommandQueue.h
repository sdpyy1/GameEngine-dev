#pragma once
#include <cstdint>
#include <vector>
namespace GameEngine {
	struct RenderCommandDebugInfo
	{
		const char* file;
		int line;
		const char* function;
	};
	class RenderCommandQueue
	{
	public:
		using RenderCommandFn = void(*)(void*);

		RenderCommandQueue();
		~RenderCommandQueue();

		void* Allocate(RenderCommandFn func, uint32_t size);
		std::vector<RenderCommandDebugInfo> m_DebugInfos; // µ÷ÊÔÐÅÏ¢

		void Execute();
	private:
		uint8_t* m_CommandBuffer;
		uint8_t* m_CommandBufferPtr;
		uint32_t m_CommandCount = 0;

	};
}