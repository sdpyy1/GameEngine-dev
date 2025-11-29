#pragma once
#include "macro.h"
#include <memory>

using byte = uint8_t;

// Pointer wrappers
template<typename T>
using Scope = std::unique_ptr<T>;
template<typename T, typename ... Args>
constexpr Scope<T> CreateScope(Args&& ... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}
