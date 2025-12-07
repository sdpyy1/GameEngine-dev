#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace GameEngine {

	class HaltonUtils
	{
	public:
		static glm::vec2 GetJitter(uint32_t frameIndex)
		{
			return glm::vec2(
				static_cast<float>(Halton2(frameIndex) - 0.5),
				static_cast<float>(Halton3(frameIndex) - 0.5)
			);
		}

		static glm::mat4 JitterProjection(const glm::mat4& proj, uint32_t frameIndex, float renderWidth, float renderHeight)
		{
			glm::mat4 jittered = proj;

			glm::vec2 jitter = GetJitter(frameIndex);

			float offsetX = jitter.x * 2.0f / renderWidth;
			float offsetY = jitter.y * 2.0f / renderHeight;

			jittered[2][0] += offsetX;
			jittered[2][1] += offsetY;

			return jittered;
		}

	private:
		static double Halton2(uint32_t index)
		{
			double result = 0.0;
			double f = 0.5;
			uint32_t i = index;
			while (i > 0)
			{
				result += f * (i % 2);
				i /= 2;
				f *= 0.5;
			}
			return result;
		}

		static double Halton3(uint32_t index)
		{
			double result = 0.0;
			double f = 1.0 / 3.0;
			uint32_t i = index;
			while (i > 0)
			{
				result += f * (i % 3);
				i /= 3;
				f /= 3.0;
			}
			return result;
		}
	};

} // namespace GameEngine
