#include "pch.h"
#include "Chat.h"

#include "StrConv.h"
#include "../resource.h"

#define IDT_CHECKCONN 1

namespace wc {
	struct ConnDlgInput {
		Chat* chat;
		int xPos, yPos;
	};

	Chat::Chat(std::wstring address, std::wstring screenname)
		: m_address(address)
		, m_screenname(screenname)
		, m_connected(false)
		, m_connectionError(false)
	{

	}

	void Chat::run(int xPos, int yPos) {
		//Run net thread to connect and communicate
		std::thread netThread(&Chat::runNetThread, this);

		//Show connection dialog until net thread connects
		ConnDlgInput* input = new ConnDlgInput{ this, xPos, yPos };
		DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOGCONNECTING), nullptr, reinterpret_cast<DLGPROC>(connDlgProc), reinterpret_cast<LPARAM>(input));

		//If we're connected, open the chat window
		if (m_connected)
			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOGCHAT), nullptr, reinterpret_cast<DLGPROC>(chatDlgProc), reinterpret_cast<LPARAM>(this));

		netThread.join();
	}

	void Chat::runNetThread() {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		addrinfo* destInfo = nullptr;
		if (getaddrinfo(toStr(m_address).c_str(), "9430", nullptr, &destInfo) != 0) {
			MessageBox(nullptr, L"Error: Could not resolve host or invalid address!", L"Error", MB_ICONERROR);
			m_connectionError = true;
			return;
		}

		SOCKET sock = socket(destInfo->ai_family, SOCK_STREAM, NULL);
		if (sock == INVALID_SOCKET) {
			MessageBox(nullptr, L"Error: could not create network socket!", L"Error", MB_ICONERROR);
			freeaddrinfo(destInfo);
			m_connectionError = true;
			return;
		}

		if (connect(sock, destInfo->ai_addr, static_cast<int>(destInfo->ai_addrlen)) != 0) {
			std::wstring errorStr = getErrorString();
			MessageBox(nullptr, std::format(L"Error: Could not connect - {}", errorStr).c_str(), L"Error", MB_ICONERROR);
			freeaddrinfo(destInfo);
			closesocket(sock);
			m_connectionError = true;
			return;
		}

		freeaddrinfo(destInfo);
		m_connected = true;

		std::string data;
		while (data != "end") {
			std::getline(std::cin, data);
			send(sock, data.data(), static_cast<int>(data.size()), NULL);
		}

		closesocket(sock);
	}

	std::wstring Chat::getErrorString() {
		int errorCode = WSAGetLastError();
		std::wstring out;
		switch (errorCode) {
			case WSAETIMEDOUT:
				out = L"Timed out";
				break;

			case WSAECONNREFUSED:
				out = L"Connection refused (WinChat may not be running on remote host)";
				break;

			case WSAENETUNREACH:
			case WSAEHOSTUNREACH:
				out = L"Remote host unreachable (You may not be connected to the internet)";
				break;

			case WSAEADDRNOTAVAIL:
				out = L"Not a valid address to connect to";
				break;

			default:
				out = std::format(L"Error Code {}", errorCode);
				break;
		}

		return out;
	}

	BOOL CALLBACK Chat::connDlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
		static Chat* chat = nullptr;

		switch (msg) {
			case WM_INITDIALOG: {
				ConnDlgInput* in = reinterpret_cast<ConnDlgInput*>(lParam);
				auto [chatTemp, x, y] = *in;
				delete in;
				chat = chatTemp;
				SetWindowPos(dlg, nullptr, x + 100, y + 100, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				SetDlgItemText(dlg, IDC_STATICADDRESS, chat->m_address.c_str());
				SendDlgItemMessage(dlg, IDC_PROGRESS, PBM_SETMARQUEE, TRUE, NULL);
				SetTimer(dlg, IDT_CHECKCONN, 100, nullptr);

				return TRUE;
			}

			case WM_TIMER:
				if (wParam == IDT_CHECKCONN && (chat->m_connected || chat->m_connectionError)) {
					EndDialog(dlg, 0);
					return TRUE;
				}
		}

		return FALSE;
	}

	BOOL CALLBACK Chat::chatDlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam) {
		static Chat* chat = nullptr;

		switch (msg) {
			case WM_INITDIALOG:
				chat = reinterpret_cast<Chat*>(lParam);
				SetWindowText(dlg, std::format(L"remote screenname at {} - WinChat", chat->m_address).c_str());
				SendMessage(dlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICONMAIN))));
				return TRUE;

			case WM_CLOSE:
				EndDialog(dlg, 0);
				return TRUE;
		}

		return FALSE;
	}


	Chat::~Chat() {

	}
}