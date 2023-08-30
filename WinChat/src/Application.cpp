#include "Application.h"

#include "../resource.h"

namespace wc {
	Application::Application(std::string& appName, std::string& appVersion)
		: m_appName(appName)
		, m_appVersion(appVersion)
	{
		std::printf("%s %s\n", m_appName.c_str(), m_appVersion.c_str());
	}

	void Application::run() {
		HINSTANCE hInst = GetModuleHandle(NULL);
		DialogBox(hInst, MAKEINTRESOURCE(IDD_MAINDIALOG), nullptr, (DLGPROC)dlgProc);
	}

	BOOL CALLBACK Application::dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		switch (msg) {
			case WM_INITDIALOG:

				return TRUE;

			case WM_CLOSE:
				EndDialog(hWnd, 0);
				return TRUE;
		}

		return FALSE;
	}

	Application::~Application() {

	}
}