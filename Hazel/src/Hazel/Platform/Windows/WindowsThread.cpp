#include "hzpch.h"
#include "Hazel/Core/Thread.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <Windows.h>

namespace GameEngine {

	Thread::Thread(const std::string& name)
		: m_Name(name)
	{
	}

	void Thread::SetName(const std::string& name)
	{
		HANDLE threadHandle = m_Thread.native_handle();

		std::wstring wName(name.begin(), name.end());
		SetThreadDescription(threadHandle, wName.c_str());
		SetThreadAffinityMask(threadHandle, 8); // 设置亲和力掩码为8，表示将线程绑定到第4个CPU核心（从0开始计数） 这和SetName无关
	}

	void Thread::Join()
	{
		if (m_Thread.joinable())
			m_Thread.join();
	}

	ThreadSignal::ThreadSignal(const std::string& name, bool manualReset)
	{
		std::wstring str(name.begin(), name.end());
		m_SignalHandle = CreateEvent(NULL, (BOOL)manualReset, FALSE, str.c_str());
	}

	void ThreadSignal::WaitAndReset()
	{
		WaitForSingleObject(m_SignalHandle, INFINITE); // 阻塞直到事件触发
	}

	void ThreadSignal::Signal()
	{
		SetEvent(m_SignalHandle); // 触发事件，唤醒等待的线程
	}

	void ThreadSignal::Reset()
	{
		ResetEvent(m_SignalHandle);
	}

	std::thread::id Thread::GetID() const
	{
		return m_Thread.get_id();
	}

}
