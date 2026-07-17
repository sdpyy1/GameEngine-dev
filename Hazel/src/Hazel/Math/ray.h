#pragma once

#include <glm/glm.hpp>
#include "Hazel/Scene/EditorCamera.h"
#include "collision.h"
#include <glm/gtx/component_wise.hpp>

namespace GameEngine {

	struct Ray
	{
		glm::vec3 Origin, Direction;

		Ray(const glm::vec3& origin, const glm::vec3& direction)
		{
			Origin = origin;
			Direction = direction;
		}

		static Ray Zero()
		{
			return { {0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };
		}

		// 屏幕坐标->从摄像机位置处发射射线
		static Ray CastRay(EditorCamera& camera, float ndcX, float ndcY)
		{
			glm::vec4 mouseClipPos = { ndcX, ndcY, 0.0f, 1.0f }; // 近裁剪面
			glm::vec4 rayCamera = glm::inverse(camera.GetProjectionMatrix()) * mouseClipPos;
			rayCamera /= rayCamera.w;
			glm::vec4 rayWorld4 = glm::inverse(camera.GetViewMatrix()) * rayCamera;
			glm::vec3 rayDir = glm::normalize(glm::vec3(rayWorld4) - camera.GetPosition());
			glm::vec3 rayPos = camera.GetPosition();
			return { rayPos, rayDir };
		}
		bool IntersectsAABB(const AxisAlignedBox& aabb, float& t) const
		{
			glm::vec3 invD = 1.0f / Direction;

			glm::vec3 t0 = (aabb.GetMinCorner() - Origin) * invD;
			glm::vec3 t1 = (aabb.GetMaxCorner() - Origin) * invD;

			glm::vec3 tmin3 = glm::min(t0, t1);
			glm::vec3 tmax3 = glm::max(t0, t1);

			float tmin = glm::compMax(tmin3);
			float tmax = glm::compMin(tmax3);

			// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
			if (tmax < 0)
			{
				t = tmax;
				return false;
			}

			// if tmin > tmax, ray doesn't intersect AABB
			if (tmin > tmax)
			{
				t = tmax;
				return false;
			}

			t = tmin;
			return true;
		}

		bool IntersectsTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float& t) const
		{
			const glm::vec3 E1 = b - a;
			const glm::vec3 E2 = c - a;
			const glm::vec3 N = cross(E1, E2);
			const float det = -glm::dot(Direction, N);
			const float invdet = 1.f / det;
			const glm::vec3 AO = Origin - a;
			const glm::vec3 DAO = glm::cross(AO, Direction);
			const float u = glm::dot(E2, DAO) * invdet;
			const float v = -glm::dot(E1, DAO) * invdet;
			t = glm::dot(AO, N) * invdet;
			return (det >= 1e-6f && t >= 0.0f && u >= 0.0f && v >= 0.0f && (u + v) <= 1.0f);
		}

	};

}
