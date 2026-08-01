#include "hzpch.h"
#include "ShaderCompiler.h"

#include "Hazel/Core/Definations.h"

#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

namespace GameEngine {

	namespace {

		constexpr const char* SHADER_COMPILER_TAG = "ShaderCompiler";

		// 与 Assets/Shader/complie.bat 保持一致的 Stage 规则
		struct StageRule
		{
			const char* Macro;			// glsl 中出现该宏时才编译此 Stage
			const char* StageArg;		// -fshader-stage=
			const char* OutputSuffix;	// 输出 spv 的文件名后缀
			bool		NeedSpv14;		// 光追相关 Stage 需要 --target-spv=spv1.4
		};

		constexpr StageRule STAGE_RULES[] =
		{
			{ "COMPUTE_SHADER",			"comp",		"Comp",		false },
			{ "VERTEX_SHADER",			"vert",		"Vert",		false },
			{ "FRAGMENT_SHADER",		"frag",		"Frag",		false },
			{ "GEOMETRY_SHADER",		"geom",		"Geom",		false },
			{ "RAYGEN_SHADER",			"rgen",		"Rgen",		true  },
			{ "RAYMISS_SHADER",			"rmiss",	"Rmiss",	true  },
			{ "RAYCLOSEST_HIT_SHADER",	"rchit",	"Rhit",		true  },
		};

		struct CompileTask
		{
			fs::path			SourceFile;
			fs::path			OutputFile;
			const StageRule*	Rule = nullptr;
			std::string			CommandLine;
			std::string			CompilerOutput;
			bool				Success = false;
		};

		std::string ReadTextFile(const fs::path& path)
		{
			std::ifstream stream(path, std::ios::in | std::ios::binary);
			if (!stream)
				return {};

			std::string content;
			stream.seekg(0, std::ios::end);
			const std::streamoff size = stream.tellg();
			if (size > 0)
			{
				content.resize(static_cast<size_t>(size));
				stream.seekg(0, std::ios::beg);
				stream.read(content.data(), size);
			}
			return content;
		}

		std::string ToUpper(std::string text)
		{
			for (char& c : text)
				c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
			return text;
		}

		std::string ToLower(std::string text)
		{
			for (char& c : text)
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			return text;
		}

		std::string Quote(const std::string& text)
		{
			return "\"" + text + "\"";
		}

		// 以 tick 为单位返回文件最后写入时间，文件不存在返回 0
		int64_t GetWriteTicks(const fs::path& path)
		{
			std::error_code ec;
			const auto time = fs::last_write_time(path, ec);
			if (ec)
				return 0;
			return static_cast<int64_t>(time.time_since_epoch().count());
		}

		// common 目录下的文件只作为 include 使用，不单独编译
		bool IsInCommonDirectory(const fs::path& path)
		{
			for (const auto& part : path)
			{
				if (ToLower(part.string()) == "common")
					return true;
			}
			return false;
		}

		// 解析文件中的 #include，返回可定位到的依赖文件绝对路径
		std::vector<fs::path> ParseIncludes(const std::string& content, const fs::path& fileDir, const fs::path& shaderRoot)
		{
			auto isSpace = [](char c) { return c == ' ' || c == '\t' || c == '\r'; };

			std::vector<fs::path> includes;
			size_t lineBegin = 0;
			while (lineBegin <= content.size())
			{
				size_t lineEnd = content.find('\n', lineBegin);
				if (lineEnd == std::string::npos)
					lineEnd = content.size();

				size_t cursor = lineBegin;
				while (cursor < lineEnd && isSpace(content[cursor])) ++cursor;

				if (cursor < lineEnd && content[cursor] == '#')
				{
					++cursor;
					while (cursor < lineEnd && isSpace(content[cursor])) ++cursor;

					if (content.compare(cursor, 7, "include") == 0)
					{
						cursor += 7;
						while (cursor < lineEnd && isSpace(content[cursor])) ++cursor;

						if (cursor < lineEnd && (content[cursor] == '"' || content[cursor] == '<'))
						{
							const char closing = (content[cursor] == '"') ? '"' : '>';
							const size_t nameBegin = cursor + 1;
							const size_t nameEnd = content.find(closing, nameBegin);
							if (nameEnd != std::string::npos && nameEnd < lineEnd)
							{
								const std::string includeName = content.substr(nameBegin, nameEnd - nameBegin);
								if (!includeName.empty())
								{
									std::error_code ec;
									fs::path candidate = (fileDir / includeName).lexically_normal();
									if (!fs::exists(candidate, ec))
										candidate = (shaderRoot / includeName).lexically_normal();

									if (fs::exists(candidate, ec))
										includes.push_back(candidate);
								}
							}
						}
					}
				}

				if (lineEnd == content.size())
					break;
				lineBegin = lineEnd + 1;
			}

			return includes;
		}

		// 递归计算「文件自身 + 所有 include 依赖」中最新的修改时间
		class DependencyTimestampResolver
		{
		public:
			explicit DependencyTimestampResolver(fs::path shaderRoot)
				: m_ShaderRoot(std::move(shaderRoot)) {}

			int64_t Resolve(const fs::path& file)
			{
				const std::string key = MakeKey(file);

				const auto it = m_Cache.find(key);
				if (it != m_Cache.end())
					return it->second;

				// 循环 include 保护
				if (!m_Visiting.insert(key).second)
					return GetWriteTicks(file);

				int64_t newest = GetWriteTicks(file);
				const std::string content = ReadTextFile(file);
				for (const fs::path& include : ParseIncludes(content, file.parent_path(), m_ShaderRoot))
					newest = std::max(newest, Resolve(include));

				m_Visiting.erase(key);
				m_Cache[key] = newest;
				return newest;
			}

		private:
			static std::string MakeKey(const fs::path& file)
			{
				return ToLower(file.lexically_normal().generic_string());
			}

		private:
			fs::path									m_ShaderRoot;
			std::unordered_map<std::string, int64_t>	m_Cache;
			std::unordered_set<std::string>				m_Visiting;
		};

		// 执行命令行并捕获 stdout / stderr，返回进程退出码
		int RunCommand(const std::string& commandLine, std::string& output)
		{
			// cmd /c 会剥掉最外层的一对引号，所以整体再包一层
			const std::string full = "\"" + commandLine + "\" 2>&1";

			FILE* pipe = _popen(full.c_str(), "r");
			if (!pipe)
				return -1;

			char buffer[512];
			while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr)
				output += buffer;

			while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
				output.pop_back();

			return _pclose(pipe);
		}

	} // namespace

	fs::path ShaderCompiler::FindGlslcExecutable()
	{
		std::error_code ec;

		// 1. VULKAN_SDK 环境变量
		if (const char* vulkanSDK = std::getenv("VULKAN_SDK"))
		{
			const fs::path candidate = fs::path(vulkanSDK) / "Bin" / "glslc.exe";
			if (fs::exists(candidate, ec))
				return candidate;
		}

		// 2. 与 complie.bat 一致的固定路径
		{
			const fs::path candidate = "D:/context/VulkanSDK/1.4.309.0/Bin/glslc.exe";
			if (fs::exists(candidate, ec))
				return candidate;
		}

		// 3. 系统 PATH
		{
			char buffer[MAX_PATH] = {};
			if (::SearchPathA(nullptr, "glslc.exe", nullptr, MAX_PATH, buffer, nullptr) > 0)
				return fs::path(buffer);
		}

		return {};
	}

	ShaderCompileResult ShaderCompiler::CompileDirtyShaders(const ShaderCompileOptions& options)
	{
		const auto startTime = std::chrono::high_resolution_clock::now();

		ShaderCompileResult result;
		std::error_code ec;

		const fs::path shaderRoot = (options.ShaderRootDir.empty() ? fs::path(APP_SHADER_PATH) : options.ShaderRootDir).lexically_normal();
		if (!fs::exists(shaderRoot, ec))
		{
			LOG_WARN_TAG(SHADER_COMPILER_TAG, "Shader 目录不存在，跳过自动编译: {}", shaderRoot.string());
			return result;
		}

		const fs::path glslc = options.GlslcPath.empty() ? FindGlslcExecutable() : options.GlslcPath;
		if (glslc.empty())
		{
			LOG_WARN_TAG(SHADER_COMPILER_TAG, "未找到 glslc.exe，跳过 Shader 自动编译（请配置 VULKAN_SDK 环境变量）");
			return result;
		}

		// ---------- 收集需要编译的 Stage ----------
		DependencyTimestampResolver resolver(shaderRoot);
		std::vector<CompileTask> tasks;

		for (const auto& entry : fs::recursive_directory_iterator(shaderRoot, ec))
		{
			if (ec)
				break;
			if (!entry.is_regular_file())
				continue;

			const fs::path& file = entry.path();
			if (ToLower(file.extension().string()) != ".glsl")
				continue;
			if (IsInCommonDirectory(fs::relative(file, shaderRoot, ec)))
				continue;

			const std::string upperContent = ToUpper(ReadTextFile(file));
			const int64_t sourceTicks = resolver.Resolve(file);
			const fs::path spvDir = file.parent_path() / "spv";

			for (const StageRule& rule : STAGE_RULES)
			{
				if (upperContent.find(rule.Macro) == std::string::npos)
					continue;

				result.TotalStages++;

				const fs::path outputFile = spvDir / (file.stem().string() + rule.OutputSuffix + ".spv");
				if (!options.ForceRebuildAll && fs::exists(outputFile, ec) && GetWriteTicks(outputFile) >= sourceTicks)
				{
					result.UpToDate++;
					continue;
				}

				fs::create_directories(spvDir, ec);

				CompileTask task;
				task.SourceFile = file;
				task.OutputFile = outputFile;
				task.Rule = &rule;
				task.CommandLine = Quote(glslc.string())
					+ " -g -O0"
					+ " -fshader-stage=" + rule.StageArg
					+ " " + Quote(file.string())
					+ " -D" + rule.Macro
					+ (rule.NeedSpv14 ? " --target-spv=spv1.4" : "")
					+ " -o " + Quote(outputFile.string());

				tasks.push_back(std::move(task));
			}
		}

		if (tasks.empty())
		{
			result.ElapsedMs = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - startTime).count();
			LOG_INFO_TAG(SHADER_COMPILER_TAG, "Shader 均为最新，无需编译（共 {} 个 Stage，检测耗时 {:.1f} ms）", result.TotalStages, result.ElapsedMs);
			return result;
		}

		LOG_INFO_TAG(SHADER_COMPILER_TAG, "检测到 {} 个 Shader Stage 需要重新编译（共 {} 个）...", tasks.size(), result.TotalStages);

		// ---------- 编译 ----------
		uint32_t threadCount = 1;
		if (options.Parallel)
		{
			threadCount = options.MaxThreads > 0 ? options.MaxThreads : std::thread::hardware_concurrency();
			threadCount = std::max(1u, std::min<uint32_t>(threadCount, static_cast<uint32_t>(tasks.size())));
		}

		std::atomic<size_t> nextTaskIndex{ 0 };
		auto worker = [&tasks, &nextTaskIndex]()
		{
			for (;;)
			{
				const size_t index = nextTaskIndex.fetch_add(1);
				if (index >= tasks.size())
					break;

				CompileTask& task = tasks[index];
				task.Success = (RunCommand(task.CommandLine, task.CompilerOutput) == 0);
			}
		};

		if (threadCount <= 1)
		{
			worker();
		}
		else
		{
			std::vector<std::thread> threads;
			threads.reserve(threadCount);
			for (uint32_t i = 0; i < threadCount; i++)
				threads.emplace_back(worker);
			for (std::thread& thread : threads)
				thread.join();
		}

		// ---------- 汇总日志（统一在主线程输出，保证顺序）----------
		for (const CompileTask& task : tasks)
		{
			const std::string relativeSource = fs::relative(task.SourceFile, shaderRoot, ec).generic_string();
			if (task.Success)
			{
				result.Compiled++;
				LOG_INFO_TAG(SHADER_COMPILER_TAG, "  [{}] {} -> {}", task.Rule->StageArg, relativeSource, task.OutputFile.filename().string());
				if (!task.CompilerOutput.empty())
					LOG_WARN_TAG(SHADER_COMPILER_TAG, "{}", task.CompilerOutput);
			}
			else
			{
				result.Failed++;
				LOG_ERROR_TAG(SHADER_COMPILER_TAG, "  [{}] {} 编译失败:\n{}", task.Rule->StageArg, relativeSource, task.CompilerOutput);
			}
		}

		result.ElapsedMs = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - startTime).count();

		if (result.IsSuccess())
			LOG_INFO_TAG(SHADER_COMPILER_TAG, "Shader 增量编译完成：编译 {}，最新 {}，耗时 {:.1f} ms", result.Compiled, result.UpToDate, result.ElapsedMs);
		else
			LOG_ERROR_TAG(SHADER_COMPILER_TAG, "Shader 增量编译结束：成功 {}，失败 {}，最新 {}，耗时 {:.1f} ms（失败的 Shader 将继续使用旧的 spv）", result.Compiled, result.Failed, result.UpToDate, result.ElapsedMs);

		return result;
	}

}
