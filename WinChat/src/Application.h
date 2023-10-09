#pragma once
#include <string>
#include <atomic>

#include <Windows.h>

namespace wc {
	class Application {
	public:
		Application(const std::string& appName, const std::string& appVersion, const int defaultPort);

		void run();

		~Application();
	private:
		void startListen();
		static BOOL CALLBACK mainDlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);
		static BOOL CALLBACK acceptDlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);

		const std::wstring m_appName;
		const std::wstring m_appVersion;
		const int m_defaultPort;
		int m_listenPort;
		bool m_running;

		std::atomic<SOCKET> m_inSocket;
		std::wstring m_inAddress;
	};
}