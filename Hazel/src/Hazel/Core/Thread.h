#pragma once
#include <string>
#include <thread>

namespace GameEngine {

	class Thread
	{
	public:
		Thread(const std::string& name);

		// 线程执行
		template<typename Fn, typename... Args>
		void Dispatch(Fn&& func, Args&&... args)
		{
			m_Thread = std::thread(func, std::forward<Args>(args)...); // 这行会立即新线程执行传入的函数
			SetName(m_Name);
			LOG_INFO("Thread [{0}] Dispatch and Run!", m_Name);
		}

		void SetName(const std::string& name);

		// 阻塞等待线程结束
		void Join();

		std::thread::id GetID() const;
	private:
		std::string m_Name;
		std::thread m_Thread;
	};

	// 线程信号，用于线程间同步
	class ThreadSignal
	{
	public:
		// manualReset: true表示手动重置，false表示自动重置 false表示一次放行一个
		ThreadSignal(const std::string& name, bool manualReset = false);

		void WaitAndReset();
		void Signal();
		void Reset();
	private:
		void* m_SignalHandle = nullptr;
	};

}
