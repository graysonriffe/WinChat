#include "Application.h"

#include <iostream>
#include <format>

#include <CommCtrl.h>

#include "../resource.h"
#include "Chat.h"

//This pragma enables visual styles, which makes dialogs and their controls look modern.
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace wc {
	struct MainDlgInput {
		Application* app;
		int xPos, yPos;
	};

	struct MainDlgOutput {
		std::wstring address;
		std::wstring screenname;
		int xPos, yPos;
	};

	Application::Application(std::string& appName, std::string& appVersion)
		: m_appName(appName.begin(), appName.end())
		, m_appVersion(appVersion.begin(), appVersion.end())
	{
		std::wcout << std::format(L"{} {}\n", m_appName, m_appVersion);
	}

	void Application::run() {
		//First, run the main dialog to get needed input...
		INT_PTR result = NULL;
		MainDlgInput* input = new MainDlgInput{ this, -1, -1 };
		while (result = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOGMAIN), nullptr, (DLGPROC)mainDlgProc, reinterpret_cast<LPARAM>(input))) {
			//Then create a Chat with the collected input...
			MainDlgOutput* output = reinterpret_cast<MainDlgOutput*>(result);
			const auto [address, screenname, x, y] = *output;
			delete output;

			{
				Chat chat(address, screenname);
				chat.run();
			}

			//And repeat until the user exits from the main dialog.
			input = new MainDlgInput{ this, x, y };
		}
	}

	BOOL CALLBACK Application::mainDlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
		static Application* app = nullptr;

		switch (msg) {
			case WM_INITDIALOG: {
				MainDlgInput* in = reinterpret_cast<MainDlgInput*>(lParam);
				auto [appTemp, xPos, yPos] = *in;
				app = appTemp;
				delete in;

				SetWindowText(dlg, app->m_appName.c_str());
				SendMessage(dlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICONMAIN))));

				if (xPos < 0) {
					POINT pt = { };
					GetCursorPos(&pt);
					MONITORINFO mi = { };
					mi.cbSize = sizeof(mi);
					GetMonitorInfo(MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST), &mi);
					xPos = mi.rcMonitor.left + 100, yPos = mi.rcMonitor.top + 100;
				}
				SetWindowPos(dlg, nullptr, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

				SetDlgItemText(dlg, IDC_STATICTITLE, app->m_appName.c_str());
				LOGFONT lFont = { };
				lFont.lfHeight = 50;
				HFONT font = CreateFontIndirect(&lFont);
				SendMessage(GetDlgItem(dlg, IDC_STATICTITLE), WM_SETFONT, reinterpret_cast<LPARAM>(font), NULL);

				SetDlgItemText(dlg, IDC_STATICDESC, L"A simple Windows chat app");

				SendDlgItemMessage(dlg, IDC_EDITADDRESS, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Address"));
				SendDlgItemMessage(dlg, IDC_EDITSCREENNAME, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"User"));

				RegisterHotKey(dlg, 1, MOD_NOREPEAT, VK_ESCAPE);

				return TRUE;
			}

			case WM_COMMAND:
				switch (LOWORD(wParam)) {
					case IDC_BUTTONCONNECT: {
						std::wstring address;
						address.resize(GetWindowTextLength(GetDlgItem(dlg, IDC_EDITADDRESS)));
						GetDlgItemText(dlg, IDC_EDITADDRESS, address.data(), static_cast<int>(address.size() + 1));
						std::wstring screenname;
						screenname.resize(GetWindowTextLength(GetDlgItem(dlg, IDC_EDITSCREENNAME)));
						GetDlgItemText(dlg, IDC_EDITSCREENNAME, screenname.data(), static_cast<int>(screenname.size() + 1));

						RECT dlgRect = { };
						GetWindowRect(dlg, &dlgRect);
						MainDlgOutput* out = new MainDlgOutput{ address, screenname, dlgRect.left, dlgRect.top };
						EndDialog(dlg, reinterpret_cast<INT_PTR>(out));
						return TRUE;
					}

					case ID_HELP_ABOUT: {
						MessageBox(dlg, std::format(L"{} {}\nGrayson Riffe 2023", app->m_appName, app->m_appVersion).c_str(), L"About", MB_OK);
						return TRUE;
					}

					case IDC_BUTTONEXIT:
					case ID_FILE_EXIT:
						PostMessage(dlg, WM_CLOSE, NULL, NULL);
						return TRUE;
				}

				return FALSE;

			case WM_HOTKEY:
				if (wParam != 1)
					return FALSE;
				[[fallthrough]];
			case WM_CLOSE:
				EndDialog(dlg, 0);
				return TRUE;
		}

		return FALSE;
	}

	Application::~Application() {

	}
}