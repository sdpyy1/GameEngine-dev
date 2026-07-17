#pragma once
#include "Hazel/Core/Log.h"
#include <filesystem>
// 自动检测平台，不过目前只支持Windows
#ifdef _WIN32
	#ifdef _WIN64
		#define HZ_PLATFORM_WINDOWS
	#else
		#error "x86 Builds are not supported!"
#endif
#elif defined(__APPLE__) || defined(__MACH__)
	#include <TargetConditionals.h>
	#if TARGET_IPHONE_SIMULATOR == 1
		#error "IOS simulator is not supported!"
	#elif TARGET_OS_IPHONE == 1
		#define HZ_PLATFORM_IOS
		#error "IOS is not supported!"
	#elif TARGET_OS_MAC == 1
		#define HZ_PLATFORM_MACOS
		#error "MacOS is not supported!"
	#else
		#error "Unknown Apple platform!"
	#endif
#elif defined(__ANDROID__)
	#define HZ_PLATFORM_ANDROID
	#error "Android is not supported!"
#elif defined(__linux__)
	#define HZ_PLATFORM_LINUX
	#error "Linux is not supported!"
#else
	#error "Unknown platform!"
#endif

// HZ_DEBUG 在premake中定义
#ifdef HZ_DEBUG
	#if defined(HZ_PLATFORM_WINDOWS)
		#define HZ_DEBUGBREAK() __debugbreak()
	#elif defined(HZ_PLATFORM_LINUX)
		#include <signal.h>
		#define HZ_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
#endif
	#define HZ_ENABLE_ASSERTS
#else
	#define HZ_DEBUGBREAK()
#endif

// Core log macros
#define LOG_TRACE(...)    do { ::GameEngine::Log::GetCoreLogger()->trace(__VA_ARGS__); ::GameEngine::Log::AddToCache(::GameEngine::LogLevel::trace, fmt::format(__VA_ARGS__)); } while(0)
#define LOG_INFO(...)     do { ::GameEngine::Log::GetCoreLogger()->info(__VA_ARGS__);  ::GameEngine::Log::AddToCache(::GameEngine::LogLevel::info,  fmt::format(__VA_ARGS__)); } while(0)
#define LOG_WARN(...)     do { ::GameEngine::Log::GetCoreLogger()->warn(__VA_ARGS__);  ::GameEngine::Log::AddToCache(::GameEngine::LogLevel::warn,  fmt::format(__VA_ARGS__)); } while(0)
#define LOG_ERROR(...)    do { ::GameEngine::Log::GetCoreLogger()->error(__VA_ARGS__); ::GameEngine::Log::AddToCache(::GameEngine::LogLevel::error, fmt::format(__VA_ARGS__)); HZ_DEBUGBREAK();} while(0)
#define LOG_CRITICAL(...) do { ::GameEngine::Log::GetCoreLogger()->critical(__VA_ARGS__); ::GameEngine::Log::AddToCache(::GameEngine::LogLevel::critical, fmt::format(__VA_ARGS__)); } while(0)

#define _TAG_LOG(level, tag, ...) do { \
    std::string tagStr = "[" + std::string(tag) + "] ";\
    std::string content = fmt::format(__VA_ARGS__); \
    std::string msg = tagStr + content;\
    ::GameEngine::Log::GetCoreLogger()->##level(msg); \
    ::GameEngine::Log::AddToCache(::GameEngine::LogLevel::##level, msg); \
} while(0)

#define LOG_TRACE_TAG(tag, ...)  _TAG_LOG(trace,   tag, __VA_ARGS__)
#define LOG_INFO_TAG(tag, ...)   _TAG_LOG(info,    tag, __VA_ARGS__)
#define LOG_WARN_TAG(tag, ...)   _TAG_LOG(warn,    tag, __VA_ARGS__)
#define LOG_ERROR_TAG(tag, ...)  _TAG_LOG(error,   tag, __VA_ARGS__)
#define LOG_CRITICAL_TAG(tag, ...)  _TAG_LOG(critical,tag, __VA_ARGS__)
#define LOG_LINE "-------------------------------------------------------------------"
#define HZ_EXPAND_MACRO(x) x
#define HZ_STRINGIFY(x) #x  // 把 x 变成 "x"
#define HZ_STRINGIFY_MACRO(x) HZ_STRINGIFY(x)
#ifdef HZ_ENABLE_ASSERTS
	#define ASSERT(condition,...) if(!(condition)) { LOG_ERROR("Assert Failed"); HZ_DEBUGBREAK(); }
	#define VERIFY(condition,...) if(!(condition)) { LOG_ERROR("Verify Failed"); HZ_DEBUGBREAK(); }
#else
	#define VERIFY(...)
	#define ASSERT(...)
#endif


// misc
#define BIT(x) (1u << x)

/* 类型说明：
- int：普通整数类型，存储实际值，赋值时拷贝
- int&：左值引用，绑定左值（具名变量），作为别名，修改影响原对象
- int&&：右值引用，绑定右值（临时对象等），支持移动语义，减少拷贝
- auto&&：通用引用，自动推导为左值/右值引用，兼容任意值类别，用于完美转发
*/
// 把一个成员函数声明成lambda
#define HZ_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

// #define RTDEBUG
#ifdef RTDEBUG
	#define RENDER_SUBMIT(...) Renderer::Submit(__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__)
#else
	#define RENDER_SUBMIT(...) Renderer::Submit(__VA_ARGS__)
#endif