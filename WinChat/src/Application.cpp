#include "Application.h"

#include <iostream>
#include <format>

#include "../resource.h"

//This pragma enables visual styles, which makes dialogs and their controls look modern.
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace wc {
	Application::Application(std::string& appName, std::string& appVersion)
		: m_appName(appName)
		, m_appVersion(appVersion)
	{
		std::cout << std::format("{} {}", m_appName, m_appVersion);
	}

	void Application::run() {
		HINSTANCE hInst = GetModuleHandle(NULL);
		DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOGMAIN), nullptr, (DLGPROC)dlgProc, reinterpret_cast<LPARAM>(this));
	}

	BOOL CALLBACK Application::dlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
		static Application* app = nullptr;

		switch (msg) {
			case WM_INITDIALOG: {
				app = reinterpret_cast<Application*>(lParam);

				std::wstring appName(app->m_appName.begin(), app->m_appName.end());
				SetWindowText(dlg, appName.c_str());

				POINT pt = { };
				GetCursorPos(&pt);
				HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
				MONITORINFO mi = { };
				mi.cbSize = sizeof(mi);
				GetMonitorInfo(mon, &mi);

				SetWindowPos(dlg, nullptr, mi.rcMonitor.left + 100, mi.rcMonitor.top + 100, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

				SetDlgItemText(dlg, IDC_STATICTITLE, appName.c_str());

				LOGFONT lFont = { };
				lFont.lfHeight = 50;

				HFONT font = CreateFontIndirect(&lFont);
				SendMessage(GetDlgItem(dlg, IDC_STATICTITLE), WM_SETFONT, reinterpret_cast<LPARAM>(font), NULL);

				return TRUE;
			}

			case WM_COMMAND:
				switch (LOWORD(wParam)) {
					case IDC_BUTTONCONNECT:

						return TRUE;

					case ID_HELP_ABOUT: {
						std::wstring appName(app->m_appName.begin(), app->m_appName.end());
						std::wstring appVer(app->m_appVersion.begin(), app->m_appVersion.end());
						std::wstring aboutStr = std::format(L"{} {}\nGrayson Riffe 2023", appName, appVer);
						MessageBox(dlg, aboutStr.c_str(), L"About", MB_OK);
						return TRUE;
					}

					case IDC_BUTTONEXIT:
					case ID_FILE_EXIT:
						PostMessage(dlg, WM_CLOSE, NULL, NULL);
						return TRUE;
				}

				return FALSE;

			case WM_CLOSE:
				EndDialog(dlg, 0);
				return TRUE;
		}

		return FALSE;
	}

	Application::~Application() {

	}
}