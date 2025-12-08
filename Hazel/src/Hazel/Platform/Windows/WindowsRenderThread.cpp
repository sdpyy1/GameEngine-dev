#include "hzpch.h"
#include "Hazel/Core/RenderThread.h"
#include "Hazel/Renderer/RenderSystem/RenderManager.h"
#include <Windows.h>

namespace GameEngine {

	struct RenderThreadData
	{
		CRITICAL_SECTION m_CriticalSection;
		CONDITION_VARIABLE m_ConditionVariable;

		RenderThread::State m_State = RenderThread::State::Idle;
	};

	static std::thread::id s_RenderThreadID;
	static uint32_t s_RenderThreadFrameIndex = 0;
	RenderThread::RenderThread(ThreadingPolicy coreThreadingPolicy)
		: m_RenderThread("Render Thread"), m_ThreadingPolicy(coreThreadingPolicy)
	{
		m_Data = new RenderThreadData();

		if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
		{
			InitializeCriticalSection(&m_Data->m_CriticalSection); // 创建临界区
			InitializeConditionVariable(&m_Data->m_ConditionVariable); // 创建条件变量配套
		}
	}

	RenderThread::~RenderThread()
	{
		if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
			DeleteCriticalSection(&m_Data->m_CriticalSection);

		s_RenderThreadID = std::thread::id();
	}

	void RenderThread::Run()
	{
		m_IsRunning = true;
		if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded) {
			m_RenderThread.Dispatch(RenderManager::RenderThreadFunc, this);
		}
		s_RenderThreadID = m_RenderThread.GetID();

	}

	void RenderThread::Terminate()
	{
		m_IsRunning = false;
		Pump();

		if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
			m_RenderThread.Join();

		s_RenderThreadID = std::thread::id();
	}

	void RenderThread::Wait(State waitForState)
	{
		if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
			return;

		EnterCriticalSection(&m_Data->m_CriticalSection);
		while (m_Data->m_State != waitForState)
		{
			// This releases the CS so that another thread can wake it
			SleepConditionVariableCS(&m_Data->m_ConditionVariable, &m_Data->m_CriticalSection, INFINITE);
		}
		LeaveCriticalSection(&m_Data->m_CriticalSection);
	}

	void RenderThread::WaitAndSet(State waitForState, State setToState)
	{
		if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
			return;

		EnterCriticalSection(&m_Data->m_CriticalSection);
		while (m_Data->m_State != waitForState)
		{
			SleepConditionVariableCS(&m_Data->m_ConditionVariable, &m_Data->m_CriticalSection, INFINITE);
		}
		m_Data->m_State = setToState;
		WakeAllConditionVariable(&m_Data->m_ConditionVariable);
		LeaveCriticalSection(&m_Data->m_CriticalSection);
	}

	void RenderThread::Set(State setToState)
	{
		if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
			return;

		EnterCriticalSection(&m_Data->m_CriticalSection);
		m_Data->m_State = setToState;
		WakeAllConditionVariable(&m_Data->m_ConditionVariable);
		LeaveCriticalSection(&m_Data->m_CriticalSection);
	}

	void RenderThread::NextFrame()
	{
		m_AppThreadFrame++;
		s_RenderThreadFrameIndex = (s_RenderThreadFrameIndex + 1 )% FRAMES_IN_FLIGHT;
	}

	void RenderThread::BlockUntilRenderComplete()
	{
		if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
			return;

		Wait(State::Idle);
	}

	void RenderThread::Kick()
	{
		if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
		{
			Set(State::Kick);
		}
	}

	void RenderThread::Pump()
	{
		NextFrame();
		Kick();
		BlockUntilRenderComplete();
	}

	uint32_t RenderThread::RT_GetFrameIndex()
	{
		return s_RenderThreadFrameIndex;
	}

	bool RenderThread::IsCurrentThreadRT()
	{
		return s_RenderThreadID == std::this_thread::get_id();
	}


}
