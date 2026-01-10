#include "hzpch.h"
#include "RenderCommandQueue.h"
#define HZ_RENDER_TRACE(...) LOG_TRACE(__VA_ARGS__)

namespace GameEngine {

    RenderCommandQueue::RenderCommandQueue()
    {
        m_CommandBuffer = new uint8_t[10 * 1024 * 1024]; // 10mb buffer
        m_CommandBufferPtr = m_CommandBuffer;
        memset(m_CommandBuffer, 0, 10 * 1024 * 1024); // 填充0
    }

    RenderCommandQueue::~RenderCommandQueue()
    {
        delete[] m_CommandBuffer;
    }

    // 缓存命令和数据
    void* RenderCommandQueue::Allocate(RenderCommandFn fn, uint32_t size)
    {
        *(RenderCommandFn*)m_CommandBufferPtr = fn;
        m_CommandBufferPtr += sizeof(RenderCommandFn);

        *(uint32_t*)m_CommandBufferPtr = size;
        m_CommandBufferPtr += sizeof(uint32_t);

        void* memory = m_CommandBufferPtr;
        m_CommandBufferPtr += size;

        m_CommandCount++;
        return memory;
    }

    void RenderCommandQueue::Execute()
    {
        byte* buffer = m_CommandBuffer;

        for (uint32_t i = 0; i < m_CommandCount; i++)
        {
            RenderCommandFn function = *(RenderCommandFn*)buffer;
            buffer += sizeof(RenderCommandFn);

            uint32_t size = *(uint32_t*)buffer;
            buffer += sizeof(uint32_t);

#ifdef RTDEBUG
            // 打印调试信息，截取最后两级目录 + 行号 + 函数名
            if (i < m_DebugInfos.size())
            {
                auto& info = m_DebugInfos[i];

                // 处理文件路径，只保留最后两级
                std::string path = info.file ? info.file : "unknown_file";
                size_t pos = path.find_last_of("/\\"); // 找最后一个斜杠
                if (pos != std::string::npos)
                {
                    size_t pos2 = path.find_last_of("/\\", pos - 1); // 倒数第二个斜杠
                    if (pos2 != std::string::npos)
                        path = path.substr(pos2 + 1); // 保留最后两级
                    else
                        path = path.substr(pos + 1);  // 保留最后一级
                }

                std::string funcName = info.function ? info.function : "unknown_function";

                LOG_WARN("[RenderCommand] Execute ID={} from {}:{} ({})",
                    i, path, info.line, funcName);
            }
            else
            {
                LOG_WARN("[RenderCommand] Execute ID={} (no debug info)", i);
            }
#endif

            // 执行命令
            function(buffer);
            buffer += size;
        }

        m_CommandBufferPtr = m_CommandBuffer;
        m_CommandCount = 0;

        // 执行完清理调试信息
        m_DebugInfos.clear();
    }
}