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
		, m_running(true)
	{
		std::wcout << std::format(L"{} {}\n", m_appName, m_appVersion);

		//Initialize Windows Sockets 2
		WSADATA data = { };
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
			MessageBox(nullptr, L"Error: could not initialize WinSock 2!", L"Error", MB_ICONERROR);
			std::exit(1);
		}

		if (LOBYTE(data.wVersion) != 2 || HIBYTE(data.wVersion) != 2) {
			MessageBox(nullptr, L"Error: WinSock version 2.2 not available!", L"Error", MB_ICONERROR);
			std::exit(1);
		}
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

		m_running = false;
		listenThread.join();
	}

	void Application::startListen() {
		addrinfo* hostInfo = nullptr;
		getaddrinfo("::", "9430", nullptr, &hostInfo);

		SOCKET sock = socket(AF_INET6, SOCK_STREAM, NULL);
		if (sock == INVALID_SOCKET) {
			MessageBox(nullptr, L"Error: could not create network socket!", L"Error", MB_ICONERROR);
			std::exit(1);
		}

		unsigned long yes = 1;
		unsigned long no = 0;
		setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char*>(&no), sizeof(no));

		if (bind(sock, hostInfo->ai_addr, static_cast<int>(hostInfo->ai_addrlen)) != 0) {
			MessageBox(nullptr, L"Error: could not bind network socket!", L"Error", MB_ICONERROR);
			std::exit(1);
		}

		freeaddrinfo(hostInfo);
		listen(sock, 1);

		ioctlsocket(sock, FIONBIO, &yes);

		sockaddr_in6 remoteAddr = { };
		int remoteSize = sizeof(remoteAddr);
		SOCKET conSock = INVALID_SOCKET;
		int errorCode = 0;
		while (m_running) {
			conSock = accept(sock, reinterpret_cast<sockaddr*>(&remoteAddr), &remoteSize);
			errorCode = WSAGetLastError();
			if (errorCode && errorCode != WSAEWOULDBLOCK) {
				MessageBox(nullptr, L"Error: could not accept connection!", L"Error", MB_ICONERROR);
				std::exit(1);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if (errorCode == WSAEWOULDBLOCK)
				continue;

			ioctlsocket(conSock, FIONBIO, &no);

			std::string remoteStr(INET6_ADDRSTRLEN, 0);
			inet_ntop(AF_INET6, &remoteAddr.sin6_addr, remoteStr.data(), INET6_ADDRSTRLEN);
			remoteStr.resize(remoteStr.find_first_of('\0', 0));
			std::cout << std::format("Connected to: {}\n", remoteStr);

			std::string buf(1000, 0);
			int bytesRecvd = 0;
			while (bytesRecvd = recv(conSock, buf.data(), 1000, NULL)) {
				buf.resize(bytesRecvd);
				std::cout << std::format("Remote: {}\n", buf);
				buf.resize(1000);
			}

			closesocket(conSock);
		}

		closesocket(sock);
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
				LOGFONT lFont = { .lfHeight = 50 };
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
						int addressSize = GetWindowTextLength(GetDlgItem(dlg, IDC_EDITADDRESS));
						int screennameSize = GetWindowTextLength(GetDlgItem(dlg, IDC_EDITSCREENNAME));
						if (!addressSize || !screennameSize) {
							EDITBALLOONTIP balloon = { .cbStruct = sizeof(balloon), .pszTitle = L"Alert", .pszText = L"You must enter an address and screenname." };
							SendDlgItemMessage(dlg, addressSize ? IDC_EDITSCREENNAME : IDC_EDITADDRESS, EM_SHOWBALLOONTIP, NULL, reinterpret_cast<LPARAM>(&balloon));
							return TRUE;
						}

						std::wstring address;
						address.resize(addressSize);
						GetDlgItemText(dlg, IDC_EDITADDRESS, address.data(), static_cast<int>(address.size() + 1));
						std::wstring screenname;
						screenname.resize(screennameSize);
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
		WSACleanup();
	}
}