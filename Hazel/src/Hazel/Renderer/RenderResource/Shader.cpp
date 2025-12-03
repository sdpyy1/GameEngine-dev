#include "hzpch.h"
#include "Shader.h"
#include "Hazel/Utils/FileSystem.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
#include "Hazel/Core/Application.h"
namespace GameEngine{

	inline std::string ShaderFrequencyToString(ShaderFrequency freq)
	{
		switch (freq)
		{
		case SHADER_FREQUENCY_COMPUTE:       return "Comp";
		case SHADER_FREQUENCY_VERTEX:        return "Vert";
		case SHADER_FREQUENCY_FRAGMENT:      return "Frag";
		case SHADER_FREQUENCY_GEOMETRY:      return "Geom";
		case SHADER_FREQUENCY_RAY_GEN:       return "Rgen";
		case SHADER_FREQUENCY_CLOSEST_HIT:   return "Chit";
		case SHADER_FREQUENCY_RAY_MISS:      return "Rmiss";
		default: return "Unknown";
		}
	}

	std::string BuildShaderPath(const std::string& appShaderPath, const std::string& path, ShaderFrequency frequency)
	{
		size_t lastSlash = path.find_last_of("/\\");
		std::string dir, filename;

		if (lastSlash != std::string::npos) {
			dir = path.substr(0, lastSlash + 1);
			filename = path.substr(lastSlash + 1);
		}
		else {
			dir = "";
			filename = path;
		}

		std::string fullPath = appShaderPath + dir + "spv/" + filename + ShaderFrequencyToString(frequency) + ".spv";
		return fullPath;
	}

	Shader::Shader(const std::string& path, ShaderFrequency frequency, const std::string& entry) : m_Path(path), m_Entry(entry), m_Frequency(frequency)
	{
		auto fullPath = BuildShaderPath(APP_SHADER_PATH, path, frequency);
		std::vector<uint8_t> m_Data;
		FileSystem::LoadBinary(fullPath, m_Data);
		RHIShaderInfo info;
		info.code = m_Data;
		info.frequency = frequency;
		m_Shader = APP_DYNAMICRHI->CreateShader(info);
	}

 }