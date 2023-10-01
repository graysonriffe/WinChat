#include "pch.h"
#include "Chat.h"

#include "StrConv.h"
#include "../resource.h"

#define IDT_CHECKCONN 2
#define IDT_UPDATECHAT 3

namespace wc {
	struct ChatDlgInput {
		Chat* chatPtr;
		int xPos, yPos;
	};

	Chat::Chat(std::wstring address, std::wstring screenname)
		: m_address(address)
		, m_screenname(screenname)
		, m_remoteScreenname(1000, 0)
		, m_connected(false)
		, m_connectionError(false)
	{

	}

	void Chat::run(int xPos, int yPos, SOCKET socket) {
		//Run net thread to connect and communicate
		std::thread netThread(&Chat::runNetThread, this, socket);

		//Show connection dialog until net thread connects unless the listen thread already did that
		ChatDlgInput input = { this, xPos, yPos };
		if (!m_connected)
			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOGCONNECTING), nullptr, reinterpret_cast<DLGPROC>(connDlgProc), reinterpret_cast<LPARAM>(&input));

		//Either way now, if we're connected, open the chat window
		if (m_connected)
			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOGCHAT), nullptr, reinterpret_cast<DLGPROC>(chatDlgProc), reinterpret_cast<LPARAM>(&input));

		netThread.join();
	}

	void Chat::runNetThread(SOCKET inSocket) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		SOCKET sock = inSocket;
		if (sock == INVALID_SOCKET) {
			addrinfo* destInfo = nullptr;
			if (getaddrinfo(toStr(m_address).c_str(), "9430", nullptr, &destInfo) != 0) {
				MessageBox(nullptr, L"Error: Could not resolve host or invalid address!", L"Error", MB_ICONERROR);
				m_connectionError = true;
				return;
			}

			sock = socket(destInfo->ai_family, SOCK_STREAM, NULL);
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
		}

		unsigned long yes = 1;
		unsigned long no = 0;
		ioctlsocket(sock, FIONBIO, &no);

		//Send and receive screen names
		send(sock, reinterpret_cast<const char*>(m_screenname.c_str()), static_cast<int>(m_screenname.size()) * sizeof(wchar_t), NULL);
		int bytesRemoteScreenname = recv(sock, reinterpret_cast<char*>(m_remoteScreenname.data()), 1000, NULL);

		if (!bytesRemoteScreenname || WSAGetLastError() == WSAECONNRESET) {
			MessageBox(nullptr, L"The remote host denied the connection.", L"Error", MB_ICONERROR);
			m_connectionError = true;
			closesocket(sock);
			return;
		}

		m_remoteScreenname.resize(bytesRemoteScreenname / 2);
		ioctlsocket(sock, FIONBIO, &yes);

		m_connected = true;

		std::wstring recvBuffer(2000, 0);
		std::wstring sendStr;
		int bytesRecvd = 0;
		while (m_connected && !m_connectionError) {
			while (!m_sendQueue.empty()) {
				sendStr = m_sendQueue.pop();
				send(sock, reinterpret_cast<const char*>(sendStr.c_str()), static_cast<int>(sendStr.size()) * sizeof(wchar_t), NULL);
			}

			bytesRecvd = recv(sock, reinterpret_cast<char*>(recvBuffer.data()), 2000, NULL);

			if (!bytesRecvd) {
				m_connected = false;
				break;
			}

			if (WSAGetLastError() != WSAEWOULDBLOCK) {
				recvBuffer.resize(bytesRecvd / 2);
				m_recvQueue.push(recvBuffer);
				recvBuffer.assign(2000, 0);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
				ChatDlgInput* in = reinterpret_cast<ChatDlgInput*>(lParam);
				chat = in->chatPtr;

				SendMessage(dlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICONMAIN))));
				SetWindowPos(dlg, nullptr, in->xPos + 100, in->yPos + 100, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
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
			case WM_INITDIALOG: {
				ChatDlgInput* in = reinterpret_cast<ChatDlgInput*>(lParam);
				chat = in->chatPtr;

				SetWindowText(dlg, std::format(L"{} - WinChat", chat->m_remoteScreenname).c_str());
				SendMessage(dlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICONMAIN))));
				SetWindowPos(dlg, nullptr, in->xPos - 50, in->yPos - 50, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				SendDlgItemMessage(dlg, IDC_EDITCHATINPUT, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Message"));

				SetDlgItemText(dlg, IDC_EDITCHATDISPLAY, std::format(L"Connected to {} at {}", chat->m_remoteScreenname, chat->m_address).c_str());

				SetTimer(dlg, IDT_UPDATECHAT, 100, nullptr);

				return TRUE;
			}

			case WM_COMMAND:
				switch (LOWORD(wParam)) {
					case IDC_BUTTONSEND: {
						int inputLength = GetWindowTextLength(GetDlgItem(dlg, IDC_EDITCHATINPUT));
						if (!inputLength) {
							EDITBALLOONTIP balloon = { .cbStruct = sizeof(balloon), .pszTitle = L"Alert", .pszText = L"Enter your message here." };
							SendDlgItemMessage(dlg, IDC_EDITCHATINPUT, EM_SHOWBALLOONTIP, NULL, reinterpret_cast<LPARAM>(&balloon));
							return TRUE;
						}

						std::wstring buffer(inputLength, 0);
						GetDlgItemText(dlg, IDC_EDITCHATINPUT, buffer.data(), static_cast<int>(buffer.size() + 1));
						chat->m_sendQueue.push(buffer);

						chat->addMessage(dlg, std::format(L"{} - {}", chat->m_screenname, buffer).c_str());

						SetDlgItemText(dlg, IDC_EDITCHATINPUT, L"");

						return TRUE;
					}

					case IDC_BUTTONDISCONNECT:
					case IDCANCEL:
						PostMessage(dlg, WM_CLOSE, NULL, NULL);
						return TRUE;
				}

				return FALSE;

			case WM_TIMER:
				if (wParam == IDT_UPDATECHAT) {
					if (!chat->m_connected)
						EndDialog(dlg, 0);

					std::wstring remoteStr;
					while (!chat->m_recvQueue.empty()) {
						remoteStr = chat->m_recvQueue.pop();
						chat->addMessage(dlg, std::format(L"{} - {}", chat->m_remoteScreenname, remoteStr).c_str());
					}

					return TRUE;
				}

				return FALSE;

			case WM_CLOSE:
				chat->m_connected = false;
				return TRUE;
		}

		return FALSE;
	}

	void Chat::addMessage(HWND dlg, std::wstring in) {
		SCROLLINFO si = { .cbSize = sizeof(si), .fMask = SIF_POS | SIF_PAGE | SIF_RANGE };
		GetScrollInfo(GetDlgItem(dlg, IDC_EDITCHATDISPLAY), SB_VERT, &si);

		std::wstring buffer(GetWindowTextLength(GetDlgItem(dlg, IDC_EDITCHATDISPLAY)), 0);
		GetDlgItemText(dlg, IDC_EDITCHATDISPLAY, buffer.data(), static_cast<int>(buffer.size() + 1));
		buffer += std::format(L"\r\n\r\n{}", in);
		SetDlgItemText(dlg, IDC_EDITCHATDISPLAY, buffer.c_str());

		//If scrolled to the bottom before adding the text
		if (static_cast<int>(si.nPage) + si.nPos > si.nMax)
			SendDlgItemMessage(dlg, IDC_EDITCHATDISPLAY, EM_LINESCROLL, 0, si.nMax);
		else
			SendDlgItemMessage(dlg, IDC_EDITCHATDISPLAY, EM_LINESCROLL, 0, si.nPos);
	}

	Chat::~Chat() {

	}
}