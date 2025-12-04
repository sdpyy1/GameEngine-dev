#pragma once
#ifdef HZ_PLATFORM_WINDOWS
#ifndef NOMINMAX
// See github.com/skypjack/entt/wiki/Frequently-Asked-Questions#warning-c4003-the-min-the-max-and-the-macro
#define NOMINMAX
#endif
#endif
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include <sstream>
#include <array>
#include <set>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>

#include "Hazel/Core/Base.h"

#include "Hazel/Core/Log.h"

#ifdef HZ_PLATFORM_WINDOWS
#include <Windows.h>
#endif
