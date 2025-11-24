#pragma once
#include "Hazel/Renderer/RHI/RHI.h"
namespace GameEngine
{ 
	namespace V2
	{ 

		class Shader
		{
		public:
				Shader(const std::string& path, ShaderFrequency frequency, const std::string& entry = "main");
				RHIShaderRef GetRHIShader() {return m_Shader;}

		private:
			std::string m_Path;
			RHIShaderRef m_Shader;
			ShaderFrequency m_Frequency;
			std::string m_Entry;
		};
	}
	using ShaderRef = std::shared_ptr<V2::Shader>;
}
