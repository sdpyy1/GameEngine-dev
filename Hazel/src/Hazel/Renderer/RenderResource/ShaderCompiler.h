#pragma once

#include <cstdint>
#include <filesystem>

namespace GameEngine {

	// Shader 增量编译配置
	struct ShaderCompileOptions
	{
		// Shader 根目录，为空时使用 APP_SHADER_PATH
		std::filesystem::path ShaderRootDir;
		// glslc.exe 路径，为空时自动查找（VULKAN_SDK -> 默认安装路径 -> PATH）
		std::filesystem::path GlslcPath;
		// 忽略时间戳，强制全量重新编译
		bool ForceRebuildAll = false;
		// 是否多线程并行编译
		bool Parallel = true;
		// 并行线程数，0 表示使用硬件并发数
		uint32_t MaxThreads = 0;
	};

	struct ShaderCompileResult
	{
		uint32_t TotalStages = 0;	// 检测到的 Shader Stage 总数
		uint32_t UpToDate = 0;		// 无需重新编译的数量
		uint32_t Compiled = 0;		// 本次成功编译的数量
		uint32_t Failed = 0;		// 本次编译失败的数量
		double   ElapsedMs = 0.0;

		bool IsSuccess() const { return Failed == 0; }
	};

	// 启动时对 Assets/Shader 下的 glsl 做增量编译：
	// 1. 递归扫描所有 *.glsl（跳过 common 目录，它们只作为被 include 的头文件）
	// 2. 根据文件内出现的 XXX_SHADER 宏推导需要编译的 Stage
	// 3. 比较 glsl（含其 #include 依赖）与 spv 的时间戳，只编译发生变化的部分
	class ShaderCompiler
	{
	public:
		static ShaderCompileResult CompileDirtyShaders(const ShaderCompileOptions& options = ShaderCompileOptions());

		// 查找 glslc.exe，找不到返回空路径
		static std::filesystem::path FindGlslcExecutable();
	};

}
