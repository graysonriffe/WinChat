#include "pch.h"
#include "Application.h"

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
		//First, start a thread to listen for incoming connections...
		std::thread listenThread(&Application::startListen, this);

		//Then, run the main dialog to get needed input...
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

		listenThread.join();
	}

	void Application::startListen() {
		WSADATA data = { };
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
			std::cerr << std::format("Error: could not initialize WinSock 2!\n");
			std::exit(1);
		}

		if (LOBYTE(data.wVersion) != 2 || HIBYTE(data.wVersion) != 2) {
			std::cerr << std::format("Error: WinSock version 2.2 not available!\n");
			std::exit(1);
		}

		addrinfo hints = { };
		addrinfo* hostInfo = nullptr;
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
		getaddrinfo(nullptr, "9430", &hints, &hostInfo);

		SOCKET sock = socket(hostInfo->ai_family, hostInfo->ai_socktype, hostInfo->ai_protocol);
		if (sock == INVALID_SOCKET) {
			std::cerr << std::format("Error: could not create network socket!\n");
			std::exit(1);
		}

		if (bind(sock, hostInfo->ai_addr, static_cast<int>(hostInfo->ai_addrlen)) != 0) {
			std::cerr << std::format("Error: could not bind network socket!\n");
			std::exit(1);
		}

		freeaddrinfo(hostInfo);
		listen(sock, 1);

		sockaddr_storage remoteAddr = { };
		int remoteSize = sizeof(remoteAddr);
		SOCKET conSock = accept(sock, reinterpret_cast<sockaddr*>(&remoteAddr), &remoteSize);
		if (conSock == INVALID_SOCKET) {
			std::cerr << std::format("Error: could not accept connection!\n");;
			std::exit(1);
		}

		std::string remoteStr;
		remoteStr.resize(INET6_ADDRSTRLEN);
		void* addrLocation = nullptr;
		if (remoteAddr.ss_family == AF_INET) {
			addrLocation = reinterpret_cast<void*>(&reinterpret_cast<sockaddr_in*>(&remoteAddr)->sin_addr);
		}
		else if (remoteAddr.ss_family = AF_INET6) {
			addrLocation = reinterpret_cast<void*>(&reinterpret_cast<sockaddr_in6*>(&remoteAddr)->sin6_addr);
		}
		inet_ntop(remoteAddr.ss_family, addrLocation, remoteStr.data(), INET6_ADDRSTRLEN);
		std::cout << std::format("Connected to: {}\n", remoteStr);

		std::string buf(1000, 0);

		int bytesRecvd = 1;
		while (bytesRecvd = recv(conSock, buf.data(), 1000, 0)) {
			buf.resize(bytesRecvd);
			std::cout << std::format("Remote: {}\n", buf);
			buf.resize(1000);
		}

		closesocket(conSock);

		closesocket(sock);
		WSACleanup();
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