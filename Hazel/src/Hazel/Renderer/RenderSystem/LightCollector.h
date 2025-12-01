#pragma once
#include "Hazel/Scene/EditorCamera.h"
#include "Hazel/Renderer/RenderResource/RenderStruct.h"
namespace GameEngine
{
	struct CascadeData
	{
		glm::mat4 ViewProj;
		glm::mat4 View;
        glm::mat4 Projection;
		float SplitDepth;
	};

	class LightCollector
	{
	public:
		static void CollectLight();
		static void CalculateCascades(CascadeData* cascades, std::shared_ptr<EditorCamera> sceneCamera, const glm::vec3& lightDirection);

		static LightInfo GetLightInfo() { return s_LightInfo; };
	private:

		static LightInfo s_LightInfo;
	};

}

