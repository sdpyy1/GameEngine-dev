#include "hzpch.h"
#include "EditorCamera.h"

#include "Hazel/Core/Input.h"

#include <GLFW/glfw3.h>
#include "Hazel/Core/Application.h"
#include <glm/gtx/quaternion.hpp>
#include <imgui.h>

namespace GameEngine {
	Camera::Camera(const glm::mat4& projection, const glm::mat4& unReversedProjection)
		: m_ProjectionMatrix(projection), m_UnReversedProjectionMatrix(unReversedProjection)
	{
	}

	Camera::Camera(const float degFov, const float width, const float height, const float nearP, const float farP)
		: m_ProjectionMatrix(glm::perspectiveFov(glm::radians(degFov), width, height, nearP, farP)), m_UnReversedProjectionMatrix(glm::perspectiveFov(glm::radians(degFov), width, height, nearP, farP))
	{
	}
	EditorCamera::EditorCamera(const float degFov, const float width, const float height, const float nearP, const float farP)
		: Camera(glm::perspectiveFov(glm::radians(degFov), width, height, nearP, farP), glm::perspectiveFov(glm::radians(degFov), width, height, nearP, farP)), m_FocalPoint(0.0f), m_VerticalFOV(glm::radians(degFov)), m_NearClip(nearP), m_FarClip(farP), m_ViewportWidth(width), m_ViewportHeight(height)
	{
		Init();
	}

	void EditorCamera::Init()
	{
		constexpr glm::vec3 position = { -10, 10, 10 };
		m_Distance = glm::distance(position, m_FocalPoint);

		m_Yaw = 3.0f * glm::pi<float>() / 4.0f;
		m_Pitch = glm::pi<float>() / 4.0f;

		m_Position = CalculatePosition();
		const glm::quat orientation = GetOrientation();
		m_Direction = glm::eulerAngles(orientation) * (180.0f / glm::pi<float>());
		m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
		m_ViewMatrix = glm::inverse(m_ViewMatrix);
	}

	static void DisableMouse()
	{
		Input::SetCursorMode(CursorMode::Locked);
		// ImGUI
		auto& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
		io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard;
	}

	static void EnableMouse()
	{
		Input::SetCursorMode(CursorMode::Normal);
		auto& io = ImGui::GetIO();
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
		io.ConfigFlags &= ~ImGuiConfigFlags_NavNoCaptureKeyboard;
	}

	void EditorCamera::OnUpdate(const Timestep ts)
	{
		bool isMouseInViewport = m_IsMouseInViewport;
		bool isButtonPressed = Input::IsMouseButtonDown(MouseButton::Right) || Input::IsMouseButtonDown(MouseButton::Middle) || (Input::IsMouseButtonDown(MouseButton::Left) && Input::IsKeyDown(KeyCode::LeftAlt));

		// 鼠标开始控制的第一帧，初始化Delta资源
		if (isButtonPressed && isMouseInViewport && !m_IsCapturing) {
			m_IsCapturing = true;
			m_InitialMousePosition = { Input::GetMouseX(), Input::GetMouseY() };
			m_YawDelta = 0.0f;
			m_PitchDelta = 0.0f;
			m_PositionDelta = glm::vec3(0.0f);
		}

		// 结束鼠标控制
		if (!isButtonPressed)
			m_IsCapturing = false;


		if (!m_IsCapturing)
		{
			EnableMouse();
		}else {
			// 鼠标控制中的一帧
			const glm::vec2 mouse{ Input::GetMouseX(), Input::GetMouseY() };
			const glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.002f;
			if (Input::IsMouseButtonDown(MouseButton::Right) && !Input::IsKeyDown(KeyCode::LeftAlt))
			{
				m_CameraMode = CameraMode::FLYCAM;
				DisableMouse();

				const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
				const float speed = GetCameraSpeed();
				// 位置调整 Position
				if (Input::IsKeyDown(KeyCode::Q))
					m_PositionDelta -= ts.GetMilliseconds() * speed * glm::vec3{ 0.f, yawSign, 0.f };
				if (Input::IsKeyDown(KeyCode::E))
					m_PositionDelta += ts.GetMilliseconds() * speed * glm::vec3{ 0.f, yawSign, 0.f };
				if (Input::IsKeyDown(KeyCode::S))
					m_PositionDelta -= ts.GetMilliseconds() * speed * m_Direction;
				if (Input::IsKeyDown(KeyCode::W))
					m_PositionDelta += ts.GetMilliseconds() * speed * m_Direction;
				if (Input::IsKeyDown(KeyCode::A))
					m_PositionDelta -= ts.GetMilliseconds() * speed * m_RightDirection;
				if (Input::IsKeyDown(KeyCode::D))
					m_PositionDelta += ts.GetMilliseconds() * speed * m_RightDirection;

				// 旋转调整Direction
				constexpr float maxRate{ 0.12f };
				// Yaw * yawSign是为了相机倒置的时候，偏航角和鼠标的移动还是一致的
				m_YawDelta += glm::clamp(yawSign * delta.x * RotationSpeed(), -maxRate, maxRate);
				m_PitchDelta += glm::clamp(delta.y * RotationSpeed(), -maxRate, maxRate);
				m_RightDirection = glm::cross(m_Direction, glm::vec3{ 0.f, yawSign, 0.f });

				// 重新调整焦点
				const float distance = glm::distance(m_FocalPoint, m_Position);
				m_FocalPoint = m_Position + GetForwardDirection() * distance;
				m_Distance = distance;
			}
			else if (Input::IsKeyDown(KeyCode::LeftAlt))
			{
				m_CameraMode = CameraMode::ARCBALL;

				if (Input::IsMouseButtonDown(MouseButton::Middle))
				{
					DisableMouse();
					MousePan(delta);
				}
				else if (Input::IsMouseButtonDown(MouseButton::Left))
				{
					DisableMouse();
					MouseRotate(delta);
				}
				else if (Input::IsMouseButtonDown(MouseButton::Right))
				{
					DisableMouse();
					MouseZoom((delta.x + delta.y) * 0.1f);
				}
				else
					EnableMouse();
			}
			else
			{
				EnableMouse();
			}

			m_InitialMousePosition = mouse;
			m_Position += m_PositionDelta;
			m_Yaw += m_YawDelta;
			m_Pitch += m_PitchDelta;

			if (m_CameraMode == CameraMode::ARCBALL)
				m_Position = CalculatePosition();
			UpdateCameraView();
		}
	}

	float EditorCamera::GetCameraSpeed() const
	{
		float speed = m_NormalSpeed;
		if (Input::IsKeyDown(KeyCode::LeftControl))
			speed /= 2 - glm::log(m_NormalSpeed);
		if (Input::IsKeyDown(KeyCode::LeftShift))
			speed *= 2 - glm::log(m_NormalSpeed);

		return glm::clamp(speed, MIN_SPEED, MAX_SPEED);
	}

	void EditorCamera::UpdateCameraView()
	{
		const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;

		// Extra step to handle the problem when the camera direction is the same as the up vector
		const float cosAngle = glm::dot(GetForwardDirection(), GetUpDirection());
		if (cosAngle * yawSign > 0.99f)
			m_PitchDelta = 0.f;

		const glm::vec3 lookAt = m_Position + GetForwardDirection();
		m_Direction = glm::normalize(lookAt - m_Position);
		m_Distance = glm::distance(m_Position, m_FocalPoint);
		m_ViewMatrix = glm::lookAt(m_Position, lookAt, glm::vec3{ 0.f, yawSign, 0.f });

		//damping for smooth camera
		m_YawDelta *= 0.6f;
		m_PitchDelta *= 0.6f;
		m_PositionDelta *= 0.8f;
	}

	void EditorCamera::Focus(const glm::vec3& focusPoint)
	{
		m_FocalPoint = focusPoint;
		m_CameraMode = CameraMode::FLYCAM;
		if (m_Distance > m_MinFocusDistance)
		{
			m_Distance -= m_Distance - m_MinFocusDistance;
			m_Position = m_FocalPoint - GetForwardDirection() * m_Distance;
		}
		m_Position = m_FocalPoint - GetForwardDirection() * m_Distance;
		UpdateCameraView();
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		const float x = glm::min(float(m_ViewportWidth) / 1000.0f, 2.4f); // max = 2.4f
		const float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		const float y = glm::min(float(m_ViewportHeight) / 1000.0f, 2.4f); // max = 2.4f
		const float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCamera::RotationSpeed() const
	{
		return 0.3f;
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = glm::max(distance, 0.0f);
		float speed = distance * distance;
		speed = glm::min(speed, 50.0f); // max speed = 50
		return speed;
	}

	void EditorCamera::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e) { return OnMouseScroll(e); });
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		if (Input::IsMouseButtonDown(MouseButton::Right))
		{
			m_NormalSpeed += e.GetYOffset() * 0.3f * m_NormalSpeed;
			m_NormalSpeed = std::clamp(m_NormalSpeed, MIN_SPEED, MAX_SPEED);
		}
		else
		{
			MouseZoom(e.GetYOffset() * 0.1f);
			UpdateCameraView();
		}

		return true;
	}

	void EditorCamera::MousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		m_FocalPoint -= GetRightDirection() * delta.x * xSpeed * m_Distance;
		m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
	}

	void EditorCamera::MouseRotate(const glm::vec2& delta)
	{
		const float yawSign = GetUpDirection().y < 0.0f ? -1.0f : 1.0f;
		m_YawDelta += yawSign * delta.x * RotationSpeed();
		m_PitchDelta += delta.y * RotationSpeed();
	}

	void EditorCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		const glm::vec3 forwardDir = GetForwardDirection();
		m_Position = m_FocalPoint - forwardDir * m_Distance;
		if (m_Distance < 1.0f)
		{
			m_FocalPoint += forwardDir * m_Distance;
			m_Distance = 1.0f;
		}
		m_PositionDelta += delta * ZoomSpeed() * forwardDir;
	}

	glm::vec3 EditorCamera::GetUpDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetRightDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.f, 0.f, 0.f));
	}

	glm::vec3 EditorCamera::GetForwardDirection() const
	{
		// 默认方向是看向—z，通过GetOrientation()来修改朝向
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance + m_PositionDelta;
	}

	glm::quat EditorCamera::GetOrientation() const
	{
		// 如果不用负号，鼠标操作会反向
		return glm::quat(glm::vec3(-m_Pitch - m_PitchDelta, -m_Yaw - m_YawDelta, 0.0f));
	}
}