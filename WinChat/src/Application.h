#pragma once
#include <string>
#include <atomic>

#include <Windows.h>

namespace wc {
	class Application {
	public:
		Application(std::string& appName, std::string& appVersion);

		void run();

		~Application();
	private:
		void startListen();
		static BOOL CALLBACK mainDlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);

		const std::wstring m_appName;
		const std::wstring m_appVersion;
		bool m_running;

		std::atomic<SOCKET> m_inSocket;
		std::wstring m_inAddress;
	};
}