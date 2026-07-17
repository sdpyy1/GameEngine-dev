#include "hzpch.h"
#include "Hazel/Core/Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace GameEngine {
	std::deque<LogEntry> Log::s_LogCache;
	std::mutex Log::s_LogMutex;

	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;

	void Log::Init()
	{
		std::vector<spdlog::sink_ptr> logSinks;
		logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("GameEngine.log", true));

		logSinks[0]->set_pattern("%^[%T] %n: %v%$");
		logSinks[1]->set_pattern("[%T] [%l] %n: %v");

		s_CoreLogger = std::make_shared<spdlog::logger>("HAZEL", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_CoreLogger);
		s_CoreLogger->set_level(spdlog::level::trace);
		s_CoreLogger->flush_on(spdlog::level::trace);

	}

	// 缓存日志，用于ImGui绘制日志窗口
	void Log::AddToCache(LogLevel level, const std::string& message)
	{
		std::lock_guard<std::mutex> lock(s_LogMutex);
		s_LogCache.push_back({ level, message });
		if (s_LogCache.size() > 1000)
			s_LogCache.pop_front();
	}
}

