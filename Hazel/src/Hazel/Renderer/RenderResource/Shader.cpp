#include "hzpch.h"
#include "Shader.h"
#include "Hazel/Utils/FileSystem.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"

namespace GameEngine{
	Shader::Shader(const std::string& path, ShaderFrequency frequency, const std::string& entry) : m_Path(path), m_Entry(entry), m_Frequency(frequency)
	{
		std::vector<uint8_t> m_Data;
		FileSystem::LoadBinary(path, m_Data);
		RHIShaderInfo info;
		info.code = m_Data;
		info.frequency = frequency;
		m_Shader = APP_DYNAMICRHI->CreateShader(info);
	}

 }