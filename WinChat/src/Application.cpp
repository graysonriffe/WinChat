#include "pch.h"
#include "Application.h"

#include "../resource.h"
#include "Chat.h"
#include "StrConv.h"

//This pragma enables visual styles, which makes dialogs and their controls look modern.
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define IDT_CHECKINCONN 1

namespace wc {
	struct MainDlgInput {
		Application* appPtr;
		int xPos, yPos;
	};

	struct MainDlgOutput {
		std::wstring address;
		std::wstring screenname;
		int xPos, yPos;
		SOCKET inSock;
	};

	Application::Application(std::string& appName, std::string& appVersion)
		: m_appName(appName.begin(), appName.end())
		, m_appVersion(appVersion.begin(), appVersion.end())
		, m_running(true)
		, m_inSocket(INVALID_SOCKET)
		, m_inAddress()
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
		//First, start a thread to listen for incoming connections
		std::thread listenThread(&Application::startListen, this);

		//Then, run the main dialog to get needed input for outgoing connections
		INT_PTR result = NULL;
		MainDlgInput* input = new MainDlgInput{ this, -1, -1 };
		while (result = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOGMAIN), nullptr, reinterpret_cast<DLGPROC>(mainDlgProc), reinterpret_cast<LPARAM>(input))) {
			//Create a Chat with the collected input or the incoming connection
			MainDlgOutput* output = reinterpret_cast<MainDlgOutput*>(result);
			const auto [address, screenname, x, y, inSocket] = *output;
			delete output;

			{
				Chat chat(address, screenname);
				chat.run(x, y, inSocket);
			}

			//And repeat until the user exits from the main dialog
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

		while (bind(sock, hostInfo->ai_addr, static_cast<int>(hostInfo->ai_addrlen)) != 0) {
			freeaddrinfo(hostInfo);
			getaddrinfo("::", "9431", nullptr, &hostInfo);
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

			std::string remoteStr(INET6_ADDRSTRLEN, 0);
			inet_ntop(AF_INET6, &remoteAddr.sin6_addr, remoteStr.data(), INET6_ADDRSTRLEN);
			remoteStr.resize(remoteStr.find_first_of('\0', 0));

			m_inAddress = toWideStr(remoteStr);
			m_inSocket = conSock;

			//Wait here until there is no incoming connection anymore before accepting again
		}

		closesocket(sock);
	}

	BOOL CALLBACK Application::mainDlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
		static Application* app = nullptr;

		switch (msg) {
			case WM_INITDIALOG: {
				MainDlgInput* in = reinterpret_cast<MainDlgInput*>(lParam);
				auto [appPtr, xPos, yPos] = *in;
				delete in;
				app = appPtr;

				SetWindowText(dlg, app->m_appName.c_str());
				SendMessage(dlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICONMAIN))));

				if (xPos == -1 && yPos == -1) {
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

				SetTimer(dlg, IDT_CHECKINCONN, 100, nullptr);

				return TRUE;
			}

			case WM_COMMAND:
				switch (LOWORD(wParam)) {
					case IDC_BUTTONCONNECT: {
						int addressSize = GetWindowTextLength(GetDlgItem(dlg, IDC_EDITADDRESS));
						int screennameSize = GetWindowTextLength(GetDlgItem(dlg, IDC_EDITSCREENNAME));
						if (!addressSize || !screennameSize) {
							EDITBALLOONTIP balloon = { .cbStruct = sizeof(balloon), .pszTitle = L"Alert", .pszText = L"You must enter an address and screen name." };
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
						EndDialog(dlg, reinterpret_cast<INT_PTR>(new MainDlgOutput{ address, screenname, dlgRect.left, dlgRect.top, INVALID_SOCKET}));
						return TRUE;
					}

					case ID_HELP_ABOUT: {
						MessageBox(dlg, std::format(L"{} {}\nGrayson Riffe 2023", app->m_appName, app->m_appVersion).c_str(), L"About", MB_OK);
						return TRUE;
					}

					case IDC_BUTTONEXIT:
					case ID_FILE_EXIT:
					case IDCANCEL: //Escape key press
						PostMessage(dlg, WM_CLOSE, NULL, NULL);
						return TRUE;
				}

				return FALSE;

			case WM_TIMER:
				if (wParam == IDT_CHECKINCONN && app->m_inSocket != INVALID_SOCKET) {
					KillTimer(dlg, IDT_CHECKINCONN);
					RECT dlgRect = { };
					GetWindowRect(dlg, &dlgRect);
					MainDlgInput acceptInput = { app, dlgRect.left, dlgRect.top };

					INT_PTR result = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOGACCEPTCONNECTION), dlg, reinterpret_cast<DLGPROC>(acceptDlgProc), reinterpret_cast<LPARAM>(&acceptInput));

					if (result == IDCANCEL) {
						closesocket(app->m_inSocket);
						app->m_inSocket = INVALID_SOCKET;
						SetTimer(dlg, IDT_CHECKINCONN, 100, nullptr);
						return TRUE;
					}

					MainDlgOutput* out = reinterpret_cast<MainDlgOutput*>(result);

					EndDialog(dlg, reinterpret_cast<INT_PTR>(new MainDlgOutput{ app->m_inAddress, out->screenname, dlgRect.left, dlgRect.top, app->m_inSocket }));
					delete out;
					app->m_inSocket = INVALID_SOCKET;
					return TRUE;
				}

				return FALSE;

			case WM_CLOSE:
				EndDialog(dlg, 0);
				return TRUE;
		}

		return FALSE;
	}

	BOOL CALLBACK Application::acceptDlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
		static Application* app = nullptr;

		switch (msg) {
			case WM_INITDIALOG: {
				MainDlgInput* in = reinterpret_cast<MainDlgInput*>(lParam);
				app = in->appPtr;

				SendMessage(dlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICONMAIN))));
				SetWindowPos(dlg, nullptr, in->xPos + 30, in->yPos + 30, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

				SetDlgItemText(dlg, IDC_STATICREMOTEINFO, app->m_inAddress.c_str());
				SendDlgItemMessage(dlg, IDC_EDITSCREENNAME, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"User"));

				return TRUE;
			}

			case WM_COMMAND:
				switch (LOWORD(wParam)) {
					case IDOK: {
						int inputLength = GetWindowTextLength(GetDlgItem(dlg, IDC_EDITSCREENNAME));
						if (!inputLength) {
							EDITBALLOONTIP balloon = { .cbStruct = sizeof(balloon), .pszTitle = L"Alert", .pszText = L"You must enter a screen name." };
							SendDlgItemMessage(dlg, IDC_EDITSCREENNAME, EM_SHOWBALLOONTIP, NULL, reinterpret_cast<LPARAM>(&balloon));
							return TRUE;
						}

						std::wstring buffer(inputLength, 0);
						GetDlgItemText(dlg, IDC_EDITSCREENNAME, buffer.data(), static_cast<int>(buffer.size() + 1));
						EndDialog(dlg, reinterpret_cast<INT_PTR>(new MainDlgOutput{ std::wstring(), buffer, NULL, NULL, INVALID_SOCKET}));
						return TRUE;
					}

					case IDCANCEL:
						EndDialog(dlg, wParam);
						return TRUE;
				}

				return FALSE;
		}

		return FALSE;
	}

	Application::~Application() {
		WSACleanup();
	}
}