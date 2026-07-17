#pragma once

#include "Hazel/Core/Base.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)
#include <mutex>
#include <deque>

namespace GameEngine {

	enum class LogLevel
	{
		trace,
		info,
		warn,
		error,
		critical
	};

	struct LogEntry
	{
		LogLevel Level;
		std::string Message;
	};

	class Log
	{
	public:
		static void Init();

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

		// 日志缓存相关接口
		static void AddToCache(LogLevel level, const std::string& message);
		static const std::deque<LogEntry>& GetCache() { return s_LogCache; }

		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;

		// 日志缓存
		static std::deque<LogEntry> s_LogCache;
		static std::mutex s_LogMutex;
	};

} // namespace GameEngine


template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
	return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
	return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
	return os << glm::to_string(quaternion);
}


