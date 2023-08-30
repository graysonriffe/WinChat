#pragma once
#include <string>

#include <Windows.h>

namespace wc {
	class Application {
	public:
		Application(std::string& appName, std::string& appVersion);

		void run();

		~Application();
	private:
		static BOOL CALLBACK dlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);

		const std::string m_appName;
		const std::string m_appVersion;
	};
}