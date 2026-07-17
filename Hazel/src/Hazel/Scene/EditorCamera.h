#pragma once
#include "Hazel/Core/TimeStep.h"
#include "Hazel/Core/Events/KeyEvent.h"
#include "Hazel/Core/Events/MouseEvent.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE   // 把深度值范围设置为[0, 1]，而不是OpenGL的[-1, 1]
#include <glm/glm.hpp>
namespace GameEngine {
	/*
		Camera类只负责投影矩阵
	*/
	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& projection, const glm::mat4& unReversedProjection);
		Camera(const float degFov, const float width, const float height, const float nearP, const float farP);
		virtual ~Camera() = default;

		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetUnReversedProjectionMatrix() const { return m_UnReversedProjectionMatrix; }

		void SetProjectionMatrix(const glm::mat4 projection, const glm::mat4 unReversedProjection)
		{
			m_ProjectionMatrix = projection;
			m_UnReversedProjectionMatrix = unReversedProjection;
		}

		void SetPerspectiveProjectionMatrix(const float radFov, const float width, const float height, const float nearP, const float farP)
		{
			m_ProjectionMatrix = glm::perspectiveFov(radFov, width, height, nearP, farP);
			m_UnReversedProjectionMatrix = glm::perspectiveFov(radFov, width, height, nearP, farP);
		}

		void SetOrthoProjectionMatrix(const float width, const float height, const float nearP, const float farP)
		{
			m_ProjectionMatrix = glm::ortho(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, nearP, farP);
			m_UnReversedProjectionMatrix = glm::ortho(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, nearP, farP);
		}
	private:
		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
		//Currently only needed for shadow maps and ImGuizmo
		glm::mat4 m_UnReversedProjectionMatrix = glm::mat4(1.0f);
	};
	/*
		FLYCAM: FPS模式
		ARCBALL:轨迹球模式
	*/
	enum class CameraMode
	{
		NONE, FLYCAM, ARCBALL
	};

	class EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(const float degFov, const float width, const float height, const float nearP, const float farP);

		void Init();
		bool GetIsMove(){return isMove;}
		void Focus(const glm::vec3& focusPoint);
		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);
		void SetIsMouseInViewPort(bool flag) { m_IsMouseInViewport = flag; }
		bool IsActive() const { return m_IsActive; }
		void SetActive(bool active) { m_IsActive = active; }

		CameraMode GetCurrentMode() const { return m_CameraMode; }

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }

		const glm::vec3& GetFocalPoint() const { return m_FocalPoint; }

		inline void SetViewportSize(uint32_t width, uint32_t height)
		{
			if (m_ViewportWidth == width && m_ViewportHeight == height)
				return;
			SetPerspectiveProjectionMatrix(m_VerticalFOV, (float)width, (float)height, m_NearClip, m_FarClip);
			m_ViewportWidth = width;
			m_ViewportHeight = height;
		}
		uint32_t GetViewportWidth() const { return m_ViewportWidth; }
		uint32_t GetViewportHeight() const { return m_ViewportHeight; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		glm::mat4 GetViewProjection() const { return GetProjectionMatrix() * m_ViewMatrix; }
		glm::mat4 GetUnReversedViewProjection() const { return GetUnReversedProjectionMatrix() * m_ViewMatrix; }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;
		glm::mat4 GetPrevProjection(const glm::mat4 curProj)
		{
			glm::mat4 res = m_PrevProj;
			if (res == glm::mat4(0)) res = curProj;
			m_PrevProj = curProj;
			return res;
		}

		// TODO: 这种代码写的非常垃圾，一个Get函数居然会改变内部状态，多次执行Get，程序会出问题
		glm::mat4 GetPrevView(const glm::mat4 curView)
		{
			if (m_PrevView != curView) {

				isMove = true;
			}
			else {
				isMove = false;
			}
			glm::mat4 res = m_PrevView;
			if (res == glm::mat4(0)) res = curView;
			m_PrevView = curView;
			return res;
		}

		const glm::vec3& GetPosition() const { return m_Position; }

		/*
			获得相机的姿态（四元数表达）
		*/
		glm::quat GetOrientation() const;

		[[nodiscard]] float GetVerticalFOV() const { return m_VerticalFOV; }
		[[nodiscard]] float GetAspectRatio() const { return m_AspectRatio; }
		[[nodiscard]] float GetNearClip() const { return m_NearClip; }
		[[nodiscard]] float GetFarClip() const { return m_FarClip; }
		[[nodiscard]] float GetPitch() const { return m_Pitch; }
		[[nodiscard]] float GetYaw() const { return m_Yaw; }
		[[nodiscard]] float GetCameraSpeed() const;
	private:
		void UpdateCameraView();

		bool OnMouseScroll(MouseScrolledEvent& e);

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;
	private:
		glm::mat4 m_ViewMatrix;
		glm::vec3 m_Position, m_Direction, m_FocalPoint;

		// Perspective projection params
		float m_VerticalFOV, m_AspectRatio, m_NearClip, m_FarClip;

		bool m_IsActive = true;
		bool m_Panning, m_Rotating;
		glm::vec2 m_InitialMousePosition{};
		glm::vec3 m_InitialFocalPoint, m_InitialRotation;

		float m_Distance;
		float m_NormalSpeed{ 0.002f };
		/*
			pitch -> X
            yaw -> Y
            roll -> Z
		*/
		float m_Pitch, m_Yaw;
		float m_PitchDelta{}, m_YawDelta{};
		glm::vec3 m_PositionDelta{};
		glm::vec3 m_RightDirection{};

		CameraMode m_CameraMode{ CameraMode::FLYCAM };

		float m_MinFocusDistance{ 100.0f };

		uint32_t m_ViewportWidth = 1280, m_ViewportHeight = 720;

		constexpr static float MIN_SPEED{ 0.0005f }, MAX_SPEED{ 2.0f };
		friend class EditorLayer;

		// m_IsCapturing = true时开始监听鼠标的移动
		bool m_IsCapturing = false;
		bool m_IsMouseInViewport = true;

		bool isMove = false;
		glm::mat4 m_PrevProj = glm::mat4(0);
        glm::mat4 m_PrevView = glm::mat4(0);

	};
	using EditorCameraRef = std::shared_ptr<EditorCamera>;
}